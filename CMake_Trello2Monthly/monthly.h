#pragma once

#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <set>
#include <unordered_map>
#include <fstream>
#include "rapidjson/document.h"
#include <optional>
#include "cpptoml.h"
#include <regex>
#include <filesystem>
#include <cstdio>
#include <algorithm>

// Current file version
constexpr auto version = "v1.1.0";

class monthly
{
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
		std::set<std::string> labels;
		float hour{};
		std::string description;
	};

	std::optional<utility::string_t> trello_secrect_;
	std::string author_;
	std::optional<std::string> date_;
	std::string filename_;

	// Create http_client to send the request.
	std::shared_ptr<web::http::client::http_client> client_;

	std::shared_ptr<std::unordered_map<std::string, std::string>> file_name_map_;

	std::unordered_map<std::string, std::string> special_characters_;

	std::vector<std::string> split_description(const std::string& input) const;

	std::optional<std::string> get_date(const std::string& board_name) const;

	std::string make_header() const;

	bool check_has_custom_field(const std::string& board_id);

	std::string get_active_boards();

	std::vector<list_info> get_lists(const std::string& board_id);

	std::vector<card_info> get_card(const std::string& list_id);

	std::vector<std::string> get_labels(const std::string& board_id);

	std::set<std::string> get_using_label(std::vector<card_info> cards);

	bool start_console_log();

	bool start_file_log(std::string filename);

	std::optional<utility::string_t> parse_config();

	std::unordered_map<std::string, std::string> create_filename_map() const;

	void process_data();

	std::unordered_map<std::string, std::string> map_special_characters() const;

	std::string sanitize_input(std::string input) const;
public:
	std::shared_ptr<spdlog::logger> console{};
	std::shared_ptr<spdlog::logger> file{};

	void initialize();

	void run();

	void shutdown();
};