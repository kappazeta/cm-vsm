// GML vector layer in XML format
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

#include "vector/cvat.hpp"

#include <expat.h>
#include <iostream>
#include <fstream>
#include <vector>


class GMLFeature {
public:
	GMLFeature();
	~GMLFeature();

	//! \todo Support multiple polygons (with cut-outs).

	std::string fid;
	std::vector<FVertex> coordinates;
	int dn;
	bool inner_boundary;
};

class GMLConverter {
public:
	GMLConverter();
	~GMLConverter();

	void set_classes(const std::vector<std::string> &classes, const std::vector<int> &include_classes);
	void set_meta_info(const std::string &task_name, const std::string &owner_username, const std::string &owner_email);
	void set_multiplier(float f);

	bool convert(const std::string &path_in, const std::string &path_out);

	void parse_polygon(const std::string &content, std::vector<FVertex> *coordinates);

	static void XMLCALL tag_start_handler(void *user_data, const XML_Char *el, const XML_Char **attr);
	static void XMLCALL tag_data_handler(void *user_data, const XML_Char *s, int len);
	static void XMLCALL tag_end_handler(void *user_data, const XML_Char *el);


	std::vector<GMLFeature *> features;
	GMLFeature *last_feature;
	std::string last_tag;

	CVATXML cvat_xml;

protected:
	float multiplier;

	std::ifstream file_in;
	std::ofstream file_out;

	std::vector<std::string> classes;
	std::vector<int> include_classes;
};

