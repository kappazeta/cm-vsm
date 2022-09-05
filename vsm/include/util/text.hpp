//! @file
//! @brief Utilities for operating with text
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

/**
 * @brief Check if the string starts with a specific prefix.
 * @param[in] text Reference to the input text.
 * @param[in] beginning Reference to the prefix to check for in the text.
 * @return True if the text starts with the prefix, otherwise false.
 */
bool startswith(std::string const &text, std::string const &beginning);

/**
 * @brief Check if the string ends with a specific suffix.
 * @param[in] text Reference to the input text.
 * @param[in] ending Reference to the suffix to check for in the text.
 * @return True if the text ends with the suffix, otherwise false.
 */
bool endswith(std::string const &text, std::string const &ending);

/**
 * @brief Translates text to lowercase.
 * @param[in] text Reference to the input text.
 * @return Text with all letters in lowercase.
 */
std::string tolower(std::string const &text);

/**
 * @brief Translates text to uppercase.
 * @param[in] text Reference to the input text.
 * @return Text with all letters in uppercase.
 */
std::string toupper(std::string const &text);

/**
 * @brief Split a string into tokens by a delimiter.
 * @param[in] text Reference to the input text.
 * @param delim Single character delimiter to use for splitting the text.
 * @return An std::vector of token strings.
 */
std::vector<std::string> split_str(std::string const &text, char delim);

/**
 * @brief Get index and first date for .nc names from an ESA Sentinel-2 product file path.
 * @param[in] path Reference to the path to the Sentinel-2 product file, for example: `/home/user/Documents/work/S2A_MSIL1C_20170815T102021_N0205_R065_T32TMR_20200905T100047.SAFE/GRANULE/L1C_T32TMR_A011216_20170815T102513/IMG_DATA/T32TMR_20170815T102021_B03.jp2`.
 * @return A string of format `INDEX_DATE`, where `INDEX` is the product tile index, and `DATE` is the acquisition date.
 */
std::string extract_index_date(const std::filesystem::path &path);

/**
 * @brief Gets tile ids from its folder name.
 * To be used on existing sub-tiles.
 * @param[in] path Reference to the path to the Sentinel-2 product file, for example: `/home/user/Documents/work/S2A_MSIL2A_20200529T094041_N0214_R036_T35VLF_20200529T120441.CVAT/tile_256_3584`
 * @return A string with tile index, for example: `tile_256_3584`.
 */
std::string extract_tile_id(const std::filesystem::path &path);
