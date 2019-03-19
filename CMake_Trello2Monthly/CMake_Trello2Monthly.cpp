// CMake_Trello2Monthly.cpp : Defines the entry point for the application.
//

#include "CMake_Trello2Monthly.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

class monthly
{
private:
	struct boards_info
	{
		std::string name;
		std::string id;
	};

	struct list_info
	{
		std::string name;
		std::string id;
	};

	struct card_info
	{
		std::string name;
		std::string label;
	};
	string_t trello_secrect_;

	static std::string make_header(const std::string& author_string, const std::string& date_string)
	{
		std::string header =
			"\\documentclass[12pt]{article}\n"
			"\\usepackage[a4paper, bottom = 1.0in, left = 1.5in, right = 1.5in]{geometry}\n"
			"\%More info : https://tex.stackexchange.com/a/12408\n"
			"\\usepackage[hidelinks]{hyperref}\n"
			"\n"
			"\%More info : https://tex.stackexchange.com/a/29594\n"
			"\\usepackage{titling}\n"
			"\\setlength{\\droptitle}{-10em}\n"
			"\n"
			"\\setlength{\\footnotesep}{\\baselineskip}\n"
			"\n"
			"\%More info : https://tex.stackexchange.com/a/136531\n"
			"\\makeatletter\n"
			"\\renewcommand{\\@seccntformat}[1]{\n"
			"  \\ifcsname prefix@#1\\endcsname\n"
			"	\\csname prefix@#1\\endcsname\n"
			"  \\else\n"
			"	\\csname the#1\\endcsname\\quad\n"
			"  \\fi\n"
			"  }\n"
			"\\newcommand\\prefix@section{For the week of }\n"
			"\\makeatother\n"
			"\n"
			"\\title{Monthly Status Report}\n";

		header += "\\author{";
		header += author_string;
		header += "}\n";
		header += "\\date";
		header += "{";
		header += date_string;
		header += "}\n";
		header += "\n";
		header += "\\begin{document}\n";
		header += "\\newpage\n";
		header += "\\maketitle\n";

		return header;
	}

	std::string get_active_boards()
	{
		// Create http_client to send the request.
		http_client client(U("https://api.trello.com"));

		// Build request URI and start the request.
		uri_builder builder;
		builder.set_path(U("/1/members/me/boards"));
		builder.append_path(trello_secrect_);

		pplx::task<std::string> request_task = client.request(methods::GET, builder.to_string())

			// Handle response headers arriving.
			.then([=](http_response response)
		{
			console->info("Received response status code from Boards querry: {}.", response.status_code());

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
			//for (const auto& boards : input)
			for (size_t i = 0; i < input.size(); ++i)
			{
				console->info("[{}] board: \"{}\" is active.", i, input.at(i).name, input.at(i).id);
			}

			console->info("Please enter board number you wish to convert to TEX:");

			int choice;
			std::cin >> choice;

			// Return the chosen board ID
			return input.at(choice).id;
		});

		// Wait for all the outstanding I/O to complete and handle any exceptions
		try
		{
			// ReSharper disable once CppExpressionWithoutSideEffects
			request_task.wait();
		}
		catch (const std::exception &e)
		{
			console->critical("Error exception: {}", e.what());
		}
		return request_task.get();
	}

	std::vector<list_info> get_lists(const std::string& board_id)
	{
		// Create http_client to send the request.
		http_client client(U("https://api.trello.com"));

		// Build request URI and start the request.
		uri_builder builder;
		builder.set_path(U("/1/boards/"));
		builder.append_path(conversions::to_string_t(board_id));
		builder.append_path(U("/lists"));
		builder.append_path(trello_secrect_);

		pplx::task<std::vector<list_info>> request_task = client.request(methods::GET, builder.to_string())

			// Handle response headers arriving.
			.then([=](http_response response)
		{
			console->info("Received response status code from List querry: {}.", response.status_code());

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
		catch (const std::exception &e)
		{
			console->critical("Error exception: {}", e.what());
		}
		return request_task.get();
	}

	// Get all the cards and its label, within a specific list
	std::vector<card_info> get_card(const std::string& list_id)
	{
		// Create http_client to send the request.
		http_client client(U("https://api.trello.com"));

		// Build request URI and start the request.
		uri_builder builder;
		builder.set_path(U("/1/lists/"));
		builder.append_path(conversions::to_string_t(list_id));
		builder.append_path(U("/cards"));
		builder.append_path(trello_secrect_);

		pplx::task<std::vector<card_info>> request_task = client.request(methods::GET, builder.to_string())

			// Handle response headers arriving.
			.then([=](http_response response)
		{
			console->info("Received response status code from Card querry: {}.", response.status_code());

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
				// Get the first label's name
				temp_card.label = object["labels"].GetArray().Begin()->FindMember("name")->value.GetString();
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
		catch (const std::exception &e)
		{
			console->critical("Error exception: {}.", e.what());
		}
		return request_task.get();
	}

	// The number of subsection in the latex will depends on the number of labels
	std::vector<std::string> get_labels(const std::string& board_id)
	{
		// Create http_client to send the request.
		http_client client(U("https://api.trello.com"));

		// Build request URI and start the request.
		uri_builder builder;
		builder.set_path(U("/1/boards/"));
		builder.append_path(conversions::to_string_t(board_id));
		builder.append_path(U("/labels/"));
		builder.append_path(trello_secrect_);

		pplx::task<std::vector<std::string>> request_task = client.request(methods::GET, builder.to_string())

			// Handle response headers arriving.
			.then([=](http_response response)
		{
			console->info("Received response status code from Label querry: {}", response.status_code());

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
		catch (const std::exception &e)
		{
			console->critical("Error exception: {}", e.what());
		}
		return request_task.get();
	}

	// Get all the labels that are used by the cards.
	// As latex is really picky about empty bullet point elements so this is done to make sure
	// that there is at least a card that was tagged with the label in order to make a "\subsubsection"
	// Also, due to the fact that labels are defined per board not per list so we cannot get label for specific list
	std::unordered_set<std::string> get_using_label(std::vector<card_info> cards) const
	{
		std::unordered_set<std::string> unique_labels;
		for (const auto& card : cards)
		{
			unique_labels.insert(card.label);
		}
		return unique_labels;
	}

	bool start_logger()
	{
		try
		{
			// Console sink
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::info);
			console_sink->set_pattern("[%^%l%$] %v");

			console = std::make_shared<spdlog::logger>("console_sink", console_sink);

			// File sink
			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Monthly Status Report.tex", true);
			file_sink->set_level(spdlog::level::info);
			file_sink->set_pattern("%v");

			file = std::make_shared<spdlog::logger>("file_sink", file_sink);
			file->flush_on(spdlog::level::info);
		}
		catch (const spdlog::spdlog_ex &ex)
		{
			std::cout << "Log init failed: " << ex.what() << std::endl;
			return false;
		}
		return true;
	}

	string_t get_secrects() const
	{
		std::ifstream input("Tokens.txt");
		std::vector<std::string> file_lines;
		if (input.good())
		{
			std::string line;
			while (std::getline(input, line))
			{
				file_lines.emplace_back(line);
			}
			const auto key = conversions::to_string_t("?key=" + file_lines.at(0));
			const auto token = conversions::to_string_t("&token=" + file_lines.at(1));

			auto trello_secrect = key + token;
			return trello_secrect;
		}
		return U("");
	}

	void process_data(const std::string& author, const std::string& date)
	{
		trello_secrect_ = get_secrects();
		if (trello_secrect_.empty())
		{
			console->critical("Cannot read API keys. Please make sure \"Tokens.txt\" exists.");
			console->info("Press any key to exit.");
			return;
		}
		// Write header to file
		file->info(make_header(author, date));

		const auto board_id = get_active_boards();
		const auto labels = get_labels(board_id);
		const auto lists = get_lists(board_id);

		// Start writing each list as a section
		for (const auto& list : lists)
		{
			auto section_string = fmt::format("\\section{{{}}}", conversions::to_utf8string(list.name));
			file->info(section_string);

			// Get the cards in this list with this label
			auto cards = get_card(list.id);

			// Get only the labels that the cards in this list use
			auto available_lable = get_using_label(cards);

			file->info("\\subsection{General Development}");
			// Loop through all the labels
			for (const auto& label : labels)
			{
				// Check whether the label is used in any of the card in this list and it's not the "Hour Breakdown" label as that label will be used later
				if (available_lable.find(label) != available_lable.end() && conversions::to_utf8string(label) != "Hour Breakdown")
				{
					auto label_string = fmt::format("\\subsubsection{{{}}}", conversions::to_utf8string(label));
					file->info(label_string);

					file->info("\\begin{itemize}");

					// Loop through all the card to put it into approriate label
					for (const auto& card : cards)
					{
						// If the card is tag with the same label then put it here.
						if (conversions::to_utf8string(card.label) == conversions::to_utf8string(label))
						{
							auto temp_string = fmt::format("	\\item {}", conversions::to_utf8string(card.name));
							file->info(temp_string);
						}
					}

					file->info("\\end{itemize}");
				}
			}

			// Write hour breakdown section
			file->info("\\subsection{Hour Breakdown}");

			for (const auto& card : cards)
			{
				// If the card is tag with the same label then put it here.
				if (conversions::to_utf8string(card.label) == "Hour Breakdown")
				{
					file->info(conversions::to_utf8string(card.name));
				}
			}
		}

		// Finish writing file
		file->info("\\end{document}");

		// Convert to PDF
		std::system(R"(pdflatex "Monthly Status Report.tex")");

		// Convert to word if pandoc is installed
		std::system(R"(pandoc -s "Monthly Status Report.tex" -o "Monthly Status Report.docx")");

		// Clean up
		std::remove("Monthly Status Report.aux");
		std::remove("Monthly Status Report.log");
		std::remove("Monthly Status Report.out");
		std::remove("Monthly Status Report.aux");
	}

public:
	monthly() = default;

	void run()
	{
		start_logger();
		std::string author;
		std::string date;

		// The Resistance's leader.
		std::cout << "Please enter author's name. For example: John Connor.\n";
		std::getline(std::cin, author);

		// Skynet becomes self-aware.
		std::cout << "Please enter the month and year for the report. For example: August 1997.\n";
		std::getline(std::cin, date);

		process_data(author, date);
	}
	std::shared_ptr<spdlog::logger> console = nullptr;
	std::shared_ptr<spdlog::logger> file = nullptr;
};

int main()
{
	monthly new_month;
	new_month.run();
	std::getchar();
	return 0;
}
