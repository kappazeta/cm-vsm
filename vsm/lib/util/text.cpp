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
	// Path example: "/home/user/Documents/work/S2A_MSIL1C_20170815T102021_N0205_R065_T32TMR_20200905T100047.SAFE/GRANULE/L1C_T32TMR_A011216_20170815T102513/IMG_DATA/T32TMR_20170815T102021_B03.jp2"
	// Path example: "/home/user/Documents/work/S2A_MSIL2A_20200509T094041_N0214_R036_T35VME_20200509T111504.SAFE/GRANULE/L2A_T35VME_A025487_20200509T094035/IMG_DATA/R20m/T35VME_20200509T094041_AOT_20m.jp2"
	std::string index_firstdate_result;
	std::string path_string = path.stem().string(); 
	std::regex regexp("(T\\d+[A-Z]+)_(\\d+T\\d+)"); // Expression extracts ...index_firstdate... from a full path file name
	std::smatch matches;
	
	std::regex_search(path_string, matches, regexp);
	if (matches.size() == 3) {
		index_firstdate_result += matches.str(1); // Takes the first group (first date)
		index_firstdate_result += "_";
		index_firstdate_result += matches.str(2); // Takes the second group (index)
	}

	return index_firstdate_result;
}

std::string extract_tile_id(const std::filesystem::path &path) {
	// Path example: "/home/user/Documents/work/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.CVAT/tile_256_3584"
	std::string tile_id_result;
	std::string path_string = path.string();
	std::regex regexp("tile_(\\d+)_(\\d+)"); // Expression extracts tile_xi_yi from the path
	std::smatch matches;
	
	std::regex_search(path_string, matches, regexp); 

	if (matches.size() != 0) {
		tile_id_result += matches.str(0); // Takes the whole match
	}

	return tile_id_result;
}

std::string extract_index_firstdate(const std::filesystem::path &path) {
	// Path example: "S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441"
	std::string index_firstdate_result;
	std::string path_string = path.string(); 
	std::regex regexp("(\\d+T\\d+)_.*?(T[\\dA-Z]+)_"); // Expression extracts ...index_firstdate... from a segments.ai file name
	std::smatch matches;
	
	std::regex_search(path_string, matches, regexp);
	if (matches.size() == 3) {
		index_firstdate_result += matches.str(2); // Takes the second group (index)
		index_firstdate_result += "_";
		index_firstdate_result += matches.str(1); // Takes the first group (first date)
	}

	return index_firstdate_result;
}
