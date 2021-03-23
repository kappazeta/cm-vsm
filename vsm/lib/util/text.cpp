// Utilities for operating with text
//
// Copyright 2020 KappaZeta Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/text.hpp"
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <regex>


bool startswith(std::string const &text, std::string const &beginning) {
	// Reference used:
	//  https://stackoverflow.com/a/874160/1692112
	if (text.length() >= beginning.length() &&
			text.compare(0, beginning.length(), beginning) == 0)
		return true;
	return false;
}

bool endswith(std::string const &text, std::string const &ending) {
	// Reference used:
	//  https://stackoverflow.com/a/874160/1692112
	if (text.length() >= ending.length() &&
			text.compare(text.length() - ending.length(), ending.length(), ending) == 0)
		return true;
	return false;
}

std::string tolower(std::string const &text) {
	// Reference used:
	//  https://stackoverflow.com/a/313990/1692112
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::tolower(c); });
	return result;
}

std::string toupper(std::string const &text) {
	// Reference used:
	//  https://stackoverflow.com/a/313990/1692112
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){ return std::toupper(c); });
	return result;
}

std::vector<std::string> split_str(std::string const &text, char delim) {
	// https://stackoverflow.com/a/10861816/1692112
	std::stringstream ss(text);
	std::vector<std::string> result;

	while(ss.good()) {
		std::string substring;
		std::getline(ss, substring, delim);
		result.push_back(substring);
	}
	return result;
}

std::string extract_index_date(const std::filesystem::path &path) {
	return path.stem().string().substr(0, path.stem().string().find_last_of("_"));
}

std::string extract_tile_id(const std::filesystem::path &path) {
	std::string tile_id_result;
    std::string path_string = path.string(); 
    std::regex regexp("tile_(\\d*)_(\\d*)"); //Expression extracts ...tile_xi_yi... from path
    std::smatch matches; 
	
	std::regex_search(path_string, matches, regexp); 

	if (matches.size() != 0)
	{
		tile_id_result+= matches.str(0);
	}

	return tile_id_result;
}

std::string extract_index_firstdate(const std::filesystem::path &path) {
	std::string index_firstdate_result;
    std::string path_string = path.string(); 
    std::regex regexp("(\\d*T\\d*_).*?(T\\d.*_)"); //Expression extracts ...index_firstdate_... from a full path
    std::smatch matches; 
	
	std::regex_search(path_string, matches, regexp);
	if (matches.size() == 3)
	{
		index_firstdate_result+= matches.str(2);
		index_firstdate_result+= matches.str(1);
	}

	return index_firstdate_result;
}
