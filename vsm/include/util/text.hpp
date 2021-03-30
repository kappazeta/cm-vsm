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

#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <regex>

bool startswith(std::string const &text, std::string const &beginning);

bool endswith(std::string const &text, std::string const &ending);

std::string tolower(std::string const &text);

std::string toupper(std::string const &text);

std::vector<std::string> split_str(std::string const &text, char delim);

/**
* Gets index and first date for .nc names.
*/
std::string extract_index_date(const std::filesystem::path &path);

/**
* Gets tile ids from its folder name. (segments.ai)
*/
std::string extract_tile_id(const std::filesystem::path &path);
