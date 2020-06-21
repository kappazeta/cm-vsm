// CVAT vector layer in XML format
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
#include <vector>


class CVATXML {
public:
	CVATXML();
	~CVATXML();

	std::string task_name;

	std::string owner_username;
	std::string owner_email;

	std::string filename;

	int w, h;

	std::string cvat_header(const std::vector<std::string> &classes);
	std::string cvat_polygon(int class_index, const std::vector<FVertex> &coordinates);
	std::string cvat_footer();

	bool validate();

	std::vector<std::string> classes;
};

