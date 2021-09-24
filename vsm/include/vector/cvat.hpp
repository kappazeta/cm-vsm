//! @file
//! @brief Generator for CVAT vector XML
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

#include "util/geometry.hpp"

#include <iostream>
#include <filesystem>
#include <vector>


/**
 * @brief Class for generating a CVAT vector classification mask, in XML format.
 * https://github.com/openvinotoolkit/cvat
 */
class CVATXML {
public:
	/**
	 * @brief Initialize an empty instance.
	 */
	CVATXML();

	/**
	 * @brief De-initialize the instance.
	 */
	~CVATXML();

	std::string task_name;	///< Name of the labelling task.

	std::string owner_username;	///< Username of the task owner.
	std::string owner_email;	///< E-mail address of the task owner.

	std::string filename;	///< Filename.

	int w, h;	///< Width and height of the image which this mask applies to.

	/**
	 * @brief Generate the file header.
	 * @param[in] classes Reference to a vector of classes used in the mask.
	 * @return File header as a string.
	 */
	std::string cvat_header(const std::vector<std::string> &classes);

	/**
	 * @brief Generate a polygon.
	 * @param class_index Index of the class which this polygon belongs to.
	 * @param[in] coordinates Reference to a vector of vertices.
	 * @return Polygon as a string.
	 */
	std::string cvat_polygon(unsigned int class_index, const Polygon<float> &coordinates);

	/**
	 * @brief Generate the file footer.
	 * @return File footer as a string.
	 */
	std::string cvat_footer();

	/**
	 * @brief Perform basic validation on the parameters.
	 * @return True if valid, otherwise false.
	 */
	bool validate();

protected:
	std::vector<std::string> classes;	///< A vector of class names.
};

