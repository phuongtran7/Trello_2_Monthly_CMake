#include  "monthly.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace http;                  // Common HTTP functionality
using namespace client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

namespace fs = std::filesystem;

monthly::monthly() = default;

void monthly::initialize()
{
	start_console_log();

#ifdef __linux__ 
	/* SSL issue when on CentOS 7. Thanks Chad for help solving this.
	 * More info: https://github.com/Azure/azure-storage-cpp/issues/197
	 * https://github.com/Microsoft/cpprestsdk/issues/929
	 */
	try
	{
		// TO DO: Look for way to search for cert instead of hard code path
		http_client_config config;
		config.set_ssl_context_callback([](boost::asio::ssl::context & context)-> void {
			context.load_verify_file(std::string("/etc/pki/ca-trust/extracted/openssl/ca-bundle.trust.crt"));
			});
		client_ = std::make_shared<http_client>(U("https://api.trello.com"), config);
	}
	catch (const boost::system::system_error & e)
	{
		console->critical("Error: {} - {}", e.code().message(), e.what());
	}

#elif _WIN32
	client_ = std::make_shared<http_client>(U("https://api.trello.com"));
#endif
}

void monthly::run()
{
	console->info("+++++++++++++++++++++++++++++++++++++++++++++");
	console->info("+ Trello to Monthly. Current version {} +", version);
	console->info("+++++++++++++++++++++++++++++++++++++++++++++");

	process_data();
}

void monthly::shutdown()
{
	console->info("++++++++++++++++++++++++++++++++++++++++++++");
	console->info("+ Completed. Please press ENTER to exit. +");
	console->info("++++++++++++++++++++++++++++++++++++++++++++");

	// Release resource;
	file.reset();
	console.reset();

	// Clean up
	spdlog::drop_all();
	std::remove(fmt::format("{}", file_name_map_->at("tex")).c_str());
	std::remove(fmt::format("{}", file_name_map_->at("aux")).c_str());
	std::remove(fmt::format("{}", file_name_map_->at("log")).c_str());
	std::remove(fmt::format("{}", file_name_map_->at("out")).c_str());
	std::remove(fmt::format("{}", file_name_map_->at("synctex.gz")).c_str());
}

// Due to the way new paragraph is represented in the Card's description, there will be two newlines
// in the Card's description.p
std::vector<std::string> monthly::split_description(const std::string& input) const
{
	const std::regex expression(R"(\n\n)");

	std::vector<std::string> elems;

	for (std::sregex_token_iterator iterator(input.begin(), input.end(), expression, -1); iterator != std::sregex_token_iterator(); ++iterator)
	{
		elems.push_back(*iterator);
	}

	return elems;
}

// Parse Month-Year pair in the Board name to create the tex file
std::optional<std::string> monthly::get_date(const std::string& board_name) const
{
	const std::regex expression(R"(\b(?:Jan(?:uary)?|Feb(?:ruary)?|Mar(?:ach)|Apr(?:il)|May|Jun(?:e)|Jul(?:y)|Aug(?:ust)|Sep(?:tember)|Oct(?:ober)|Nov(?:ember)|Dec(?:ember)?) (?:19[7-9]\d|2\d{3})(?=\D|$))");

	std::smatch match;

	if (std::regex_search(board_name, match, expression))
	{
		// There should only one month-year pair in the board name
		return match[0];
	}
	return std::nullopt;
}

std::string monthly::make_header() const
{
	std::string header =
		"\\documentclass[12pt]{article}\n"
		"\\usepackage[a4paper, bottom = 1.0in, left = 1.5in, right = 1.5in]{geometry}\n"
		"\\usepackage[hidelinks]{hyperref}\n"
		"\n"
		"\\setlength{\\footnotesep}{\\baselineskip}\n"
		"\n"
		"\\makeatletter\n"
		"\\renewcommand{\\@seccntformat}[1]{\n"
		"  \\ifcsname prefix@#1\\endcsname\n"
		"   \\csname prefix@#1\\endcsname\n"
		"  \\else\n"
		"   \\csname the#1\\endcsname\\quad\n"
		"  \\fi\n"
		"  }\n"
		"\\newcommand\\prefix@section{For the week of }\n"
		"\\makeatother\n"
		"\n"
		"\\title{Monthly Status Report}\n";

	header.append(fmt::format("\\author{{{}}}\n", author_));
	header.append(fmt::format("\\date{{{}}}\n", date_.value_or("")));

	const std::string tail =
		"\n"
		"\\begin{document}\n"
		"\\maketitle\n";

	header.append(tail);

	return header;
}

bool monthly::check_has_custom_field(const std::string& board_id)
{
	// Build request URI and start the request.
	uri_builder builder;
	builder.set_path(U("/1/boards"));
	builder.append_path(conversions::to_string_t(board_id));
	builder.append_path(U("/customFields"));
	builder.append_path(trello_secrect_.value());

	pplx::task<bool> request_task = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Custom Field querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse JSON
				.then([=](std::string json_data)
					{
						rapidjson::Document document;
						document.Parse(json_data.c_str());

						// If the reponse json empty then there is no custom field
						return !document.GetArray().Empty();
					});
			// Wait for all the outstanding I/O to complete and handle any exceptions
			try
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				request_task.wait();
			}
			catch (const std::exception & e)
			{
				console->critical("Error exception: {}", e.what());
			}
			return request_task.get();
}

std::string monthly::get_active_boards()
{
	// Build request URI and start the request.
	uri_builder builder;
	builder.set_path(U("/1/members/me/boards"));
	builder.append_path(trello_secrect_.value());

	pplx::task<std::string> request_task = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Boards querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse JSON
				.then([=](std::string json_data)
					{
						rapidjson::Document document;
						document.Parse(json_data.c_str());

						std::vector<boards_info> list_of_open_boards;

						// Only get the board that the "close" value is false
						for (const auto& object : document.GetArray()) {
							if (!object.FindMember("closed")->value.GetBool())
							{
								boards_info temp;
								temp.name = object.FindMember("name")->value.GetString();
								temp.id = object.FindMember("id")->value.GetString();
								list_of_open_boards.emplace_back(temp);
							}
						}

						return list_of_open_boards;
					})

				.then([=](std::vector<boards_info> input)
					{
						// If there is only one active then proceed without user interation
						if (input.size() == 1)
						{
							console->info("Detect only one active board. Proceed without input.");
							// Set the board name as the date/month for the report
							date_ = get_date(input.at(0).name);
							// Set file name
							filename_ = input.at(0).name;
							return input.at(0).id;
						}

						for (auto i = 0; i < static_cast<int>(input.size()); ++i)
						{
							console->info("[{}] board: \"{}\" is active.", i, input.at(i).name, input.at(i).id);
						}

						console->info("Please enter board number you wish to convert to Monthly Status Report:");

						int choice;
						std::cin >> choice;
						std::cin.get();
						// Set the board name as the date/month for the report
						date_ = get_date(input.at(choice).name);
						// Set file name
						filename_ = input.at(choice).name;
						// Return the chosen board ID
						return input.at(choice).id;
					});

					// Wait for all the outstanding I/O to complete and handle any exceptions
					try
					{
						// ReSharper disable once CppExpressionWithoutSideEffects
						request_task.wait();
					}
					catch (const std::exception & e)
					{
						console->critical("Error exception: {}", e.what());
					}
					return request_task.get();
}

std::vector<monthly::list_info> monthly::get_lists(const std::string& board_id)
{
	// Build request URI and start the request.
	uri_builder builder;
	builder.set_path(U("/1/boards/"));
	builder.append_path(conversions::to_string_t(board_id));
	builder.append_path(U("/lists"));
	builder.append_path(trello_secrect_.value());

	pplx::task<std::vector<list_info>> request_task = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Lists querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse JSON
				.then([=](std::string json_data)
					{
						std::vector<list_info> list_id;

						rapidjson::Document document;
						document.Parse(json_data.c_str());

						// Loop through all the list and get the data
						for (const auto& object : document.GetArray()) {
							list_info temp_list;
							temp_list.name = object.FindMember("name")->value.GetString();
							temp_list.id = object.FindMember("id")->value.GetString();
							list_id.emplace_back(temp_list);
						}
						return list_id;
					});

			// Wait for all the outstanding I/O to complete and handle any exceptions
			try
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				request_task.wait();
			}
			catch (const std::exception & e)
			{
				console->critical("Error exception: {}", e.what());
				return {};
			}
			return request_task.get();
}

// Get all the cards and its label, within a specific list
std::vector<monthly::card_info> monthly::get_card(const std::string& list_id)
{
	// Build request URI and start the request.
	uri_builder builder;
	builder.set_path(U("/1/lists/"));
	builder.append_path(conversions::to_string_t(list_id));
	builder.append_path(U("/cards"));
	const auto custom_field_path = trello_secrect_.value() + U("&customFieldItems=true"); // For some reason append_path here return 401
	builder.append_path(custom_field_path);

	pplx::task<std::vector<card_info>> request_task = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Card querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse JSON
				.then([=](std::string json_data)
					{
						std::vector<card_info> cards;

						rapidjson::Document document;
						document.Parse(json_data.c_str());

						// Loop through all the cards
						for (const auto& object : document.GetArray()) {
							card_info temp_card;
							temp_card.name = object.FindMember("name")->value.GetString();
							temp_card.description = object.FindMember("desc")->value.GetString();
							// Get all the labels that attached to the card
							for (auto& temp_label_object : object["labels"].GetArray())
							{
								temp_card.labels.insert(temp_label_object.FindMember("name")->value.GetString());
							}
							// If there is a customField array
							if (!object["customFieldItems"].Empty())
							{
								auto custom_field_array = object["customFieldItems"].GetArray();
								for (auto& field : custom_field_array)
								{
									temp_card.hour = std::stof(field.FindMember("value")->value.FindMember("number")->value.GetString());
								}
							}
							cards.emplace_back(temp_card);
						}
						return cards;
					});

			// Wait for all the outstanding I/O to complete and handle any exceptions
			try
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				request_task.wait();
			}
			catch (const std::exception & e)
			{
				console->critical("Error exception: {}.", e.what());
			}
			return request_task.get();
}

// The number of subsection in the latex will depends on the number of labels
std::vector<std::string> monthly::get_labels(const std::string& board_id)
{
	// Build request URI and start the request.
	uri_builder builder;
	builder.set_path(U("/1/boards/"));
	builder.append_path(conversions::to_string_t(board_id));
	builder.append_path(U("/labels/"));
	builder.append_path(trello_secrect_.value());

	pplx::task<std::vector<std::string>> request_task = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Labels querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse JSON
				.then([=](std::string json_data)
					{
						std::vector<std::string> labels;

						rapidjson::Document document;
						document.Parse(json_data.c_str());

						// Loop through all the label objects
						for (const auto& object : document.GetArray()) {
							// Get the name of the label
							labels.emplace_back(object.FindMember("name")->value.GetString());
						}
						return labels;
					});

			// Wait for all the outstanding I/O to complete and handle any exceptions
			try
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				request_task.wait();
			}
			catch (const std::exception & e)
			{
				console->critical("Error exception: {}", e.what());
			}
			return request_task.get();
}

// Get all the labels that are used by the cards.
// As latex is really picky about empty bullet point elements so this is done to make sure
// that there is at least a card that was tagged with the label in order to make a "\subsubsection"
// Also, due to the fact that labels are defined per board not per list so we cannot get label for specific list
std::set<std::string> monthly::get_using_label(std::vector<card_info> cards)
{
	std::set<std::string> unique_labels;
	for (const auto& card : cards)
	{
		for (const auto& label : card.labels)
		{
			unique_labels.insert(label);
		}
	}
	return unique_labels;
}

bool monthly::start_console_log()
{
	try
	{
		// Console sink
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::info);
		console_sink->set_pattern("[%^%l%$] %v");

		console = std::make_shared<spdlog::logger>("console_sink", console_sink);
	}
	catch (const spdlog::spdlog_ex & ex)
	{
		std::cout << "Console log init failed: " << ex.what() << std::endl;
		return false;
	}
	return true;
}

bool monthly::start_file_log(std::string filename)
{
	try
	{
		// File sink
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
		file_sink->set_level(spdlog::level::info);
		file_sink->set_pattern("%v");

		file = std::make_shared<spdlog::logger>("file_sink", file_sink);
		file->flush_on(spdlog::level::info);
	}
	catch (const spdlog::spdlog_ex & ex)
	{
		std::cout << "File log init failed: " << ex.what() << std::endl;
		return false;
	}
	return true;
}

std::optional<string_t> monthly::parse_config()
{
	try
	{
		const auto config = cpptoml::parse_file("config.toml");
		const auto key_string = config->get_qualified_as<std::string>("Configuration.key").value_or("");
		const auto token_string = config->get_qualified_as<std::string>("Configuration.token").value_or("");
		const auto author_string = config->get_qualified_as<std::string>("Configuration.author").value_or("");

		if (key_string.empty() || token_string.empty() || author_string.empty())
		{
			return std::nullopt;
		}
		author_ = author_string;
		const auto key = conversions::to_string_t("?key=" + key_string);
		const auto token = conversions::to_string_t("&token=" + token_string);

		auto secrect = key + token;
		return secrect;
	}
	catch (cpptoml::parse_exception&)
	{
		return std::nullopt;
	}
}

std::unordered_map<std::string, std::string> monthly::create_filename_map() const
{
	std::vector<std::string> extensions{ "tex", "docx", "aux", "out", "log", "synctex.gz" };
	std::unordered_map<std::string, std::string> file_map;
	for (const auto& ext : extensions)
	{
		file_map[ext] = fmt::format("{}.{}", filename_, ext);
	}
	return file_map;
}

void monthly::process_data()
{
	trello_secrect_ = parse_config();
	if (!trello_secrect_.has_value())
	{
		console->critical(R"(Cannot read API keys. Please make sure "config.toml" exists.)");
		console->info("Press any key to exit.");
		return;
	}

	const auto board_id = get_active_boards();
	const auto labels = get_labels(board_id);
	const auto lists = get_lists(board_id);

	file_name_map_ = std::make_shared<std::unordered_map<std::string, std::string>>(create_filename_map());

	// Start file logger
	start_file_log(file_name_map_->at("tex"));

	// Write header to file
	file->info(make_header());

	// Start writing each list as a section
	for (const auto& list : lists)
	{
		auto section_string = fmt::format("\\section{{{}}}", list.name);
		file->info(section_string);

		// Get the cards in this list with this label
		auto cards = get_card(list.id);

		// Get only the labels that the cards in this list use
		auto available_lable = get_using_label(cards);

		file->info("\\subsection{Completed Tasks}");

		/* If there is no "Hour Breakdown" in the set of available_labels
		 * then it means that the user is tagging each of the card with the custom field
		 * work hour.
		 */
		if (available_lable.find("Hour Breakdown") == available_lable.end())
		{
			std::unordered_map<std::string, float> work_hour;

			for (const auto& label : available_lable)
			{
				auto label_string = fmt::format("\\subsubsection{{{}}}", label);
				file->info(label_string);

				file->info("\\begin{itemize}");
				// Loop through each card
				for (const auto& card : cards)
				{
					// If the card has the current label then write it down here.
					// A card can have multiple label and it will appear at multiple section.
					if (card.labels.find(label) != card.labels.end())
					{
						auto temp_string = fmt::format("    \\item {}", card.name);
						file->info(temp_string);

						// If the card has description then write it into the subitem. Thanks Al for this suggestion
						if (!card.description.empty())
						{
							auto split_input = split_description(card.description);

							// Due to converting to .docx break subitem line break so now we will use list for each of
							// desciption line.
							file->info("\\begin{itemize}");
							for (const auto& line : split_input)
							{
								auto temp_desc = fmt::format("          \\item {}", line);
								file->info(temp_desc);
							}
							file->info("\\end{itemize}");
						}

						// Save the label of the card and each work hour here for later access
						work_hour[label] += card.hour;
					}
				}
				file->info("\\end{itemize}");
			}
			// Write hour breakdown section
			file->info("\\subsection{Hour Breakdown}");
			file->info("\\begin{itemize}");
			for (const auto& item : work_hour)
			{
				file->info("        \\item {}: {} hours.\n", item.first, item.second);
			}
			file->info("\\end{itemize}");
		}
		else
		{
			// Temporary delete the "Hour Breakdown" label first
			available_lable.erase("Hour Breakdown");

			// Loop through all the labels that the cards in this list uses instead of all the labels in the board
			for (const auto& label : available_lable)
			{
				auto label_string = fmt::format("\\subsubsection{{{}}}", label);
				file->info(label_string);

				file->info("\\begin{itemize}");
				// Loop through each card
				for (const auto& card : cards)
				{
					// If the card has the current label then write it down here.
					// A card can have multiple label and it will appear at multiple sections.
					if (card.labels.find(label) != card.labels.end())
					{
						auto temp_string = fmt::format("    \\item {}", card.name);
						file->info(temp_string);

						// If the card has description then write it into the subitem. Thanks Al for this suggestion
						if (!card.description.empty())
						{
							auto split_input = split_description(card.description);

							// Due to converting to .docx break subitem line break so now we will use list for each of
							// desciption line.
							file->info("    \\begin{itemize}");
							for (const auto& line : split_input)
							{
								auto temp_desc = fmt::format("          \\item {}", line);
								file->info(temp_desc);
							}
							file->info("    \\end{itemize}");
						}
					}
				}

				file->info("\\end{itemize}");
			}

			// Write hour breakdown section
			file->info("\\subsection{Hour Breakdown}");
			file->info("\\begin{itemize}");
			// Loop through each card
			for (const auto& card : cards)
			{
				if (card.labels.find("Hour Breakdown") != card.labels.end())
				{
					file->info("        \\item {}", card.name);
				}
			}
			file->info("\\end{itemize}");
		}
	}

	// Finish writing file
	file->info("\\end{document}");

	// Convert to PDF
	std::system((fmt::format(R"(pdflatex --interaction=batchmode "{}")", file_name_map_->at("tex"))).c_str());

	// Convert to word if pandoc is installed
	std::system((fmt::format(R"(pandoc -s "{}" -o "{}")", file_name_map_->at("tex"), file_name_map_->at("docx"))).c_str());
}