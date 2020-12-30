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

#include "vector/gml.hpp"

#include <sstream>
#include <cstring>
#include <algorithm>


GMLFeature::GMLFeature(): fid(""), dn(0), inner_boundary(false) {}
GMLFeature::~GMLFeature() {
	fid.clear();
	coordinates.clear();
}


GMLConverter::GMLConverter(): last_feature(nullptr), multiplier(0.0f), file_in(), file_out() {}
GMLConverter::~GMLConverter() {}

void GMLConverter::set_classes(const std::vector<std::string> &classes, const std::vector<int> &include_classes) {
	this->classes = classes;
	this->include_classes = include_classes;
}

void GMLConverter::set_meta_info(const std::string &task_name, const std::string &owner_username, const std::string &owner_email) {
	cvat_xml.task_name = task_name;
	cvat_xml.owner_username = owner_username;
	cvat_xml.owner_email = owner_email;
}

void GMLConverter::set_multiplier(float f) {
	multiplier = f;
}

void XMLCALL GMLConverter::tag_start_handler(void *user_data, const XML_Char *el, const XML_Char **attr) {
	GMLConverter *p_inst = (GMLConverter *) user_data;

	if (!strncmp(el, "gml:featureMember", 17)) {
		p_inst->last_feature = new GMLFeature();
	} else if (!strncmp(el, "ogr:out", 7)) {
		for (int i=0; attr[i]; i+=2) {
			if (!strncmp(attr[i], "fid", 3)) {
				p_inst->last_feature->fid.assign(attr[i + 1]);
			}
		}
	} else if (!strncmp(el, "gml:outerBoundaryIs", 19)) {
		p_inst->last_feature->inner_boundary = false;
	} else if (!strncmp(el, "gml:innerBoundaryIs", 19)) {
		p_inst->last_feature->inner_boundary = true;
	}

	p_inst->last_tag.assign(el);
}

void GMLConverter::parse_polygon(const std::string &content, std::vector<FVertex> *coordinates) {
	size_t idx_sep_start = 0;
	size_t idx_sep_end = 0;
	float x, y;
	
	std::istringstream stream(content);
	std::string token, x_str, y_str;

	while (std::getline(stream, token, ' ')) {
		idx_sep_start = token.find_first_of(", ");
		idx_sep_end = token.find_last_of(", ");

		if (idx_sep_start == std::string::npos || idx_sep_end == std::string::npos)
			break;

		x_str = token.substr(0, idx_sep_start);
		y_str = token.substr(idx_sep_end + 1);
		x = atoi(x_str.c_str()) * multiplier;
		y = atoi(y_str.c_str()) * multiplier;

		coordinates->push_back(FVertex(x, y));
	}
}

void XMLCALL GMLConverter::tag_data_handler(void *user_data, const XML_Char *s, int len) {
	GMLConverter *p_inst = (GMLConverter *) user_data;
	std::string content(s, len);

	if (p_inst->last_tag == "ogr:DN") {
		p_inst->last_feature->dn = atoi(content.c_str());
	} else if (p_inst->last_tag == "gml:coordinates") {
		if (!p_inst->last_feature->inner_boundary) {
			p_inst->parse_polygon(content, &p_inst->last_feature->coordinates);
		}
	} else if (p_inst->last_tag == "gml:X") {
		int x = atoi(content.c_str());
		if (x > p_inst->cvat_xml.w)
			p_inst->cvat_xml.w = x;
	} else if (p_inst->last_tag == "gml:Y") {
		int y = atoi(content.c_str());
		if (y > p_inst->cvat_xml.h)
			p_inst->cvat_xml.h = y;
	}
}

void XMLCALL GMLConverter::tag_end_handler(void *user_data, const XML_Char *el) {
	GMLConverter *p_inst = (GMLConverter *) user_data;

	if (!strncmp(el, "gml:featureMember", 17)) {
		if (std::find(p_inst->include_classes.begin(), p_inst->include_classes.end(), p_inst->last_feature->dn)
				!= p_inst->include_classes.end())
			p_inst->file_out << p_inst->cvat_xml.cvat_polygon(p_inst->last_feature->dn, p_inst->last_feature->coordinates);

		if (p_inst->last_feature)
			delete p_inst->last_feature;
		p_inst->last_feature = nullptr;
	}
	
	p_inst->last_tag.assign("");
}

bool GMLConverter::convert(const std::string &path_in, const std::string &path_out) {
	XML_Parser p = nullptr;
	bool retval = true;
	const int buffer_size = 100 * 1024;
	char buffer[buffer_size];

	try {
		file_in.open(path_in.c_str(), std::ios_base::in);
		file_out.open(path_out.c_str(), std::ios_base::out);

		size_t last_slash = path_in.find_last_of("/\\");
		if (last_slash != std::string::npos)
			last_slash += 1;
		cvat_xml.filename = path_in.substr(last_slash);

		p = XML_ParserCreate(nullptr);

		XML_SetUserData(p, this);
		XML_SetCharacterDataHandler(p, tag_data_handler);
		XML_SetElementHandler(p, tag_start_handler, tag_end_handler);

		bool header_written = false;
		int len = 0;
		int done = 0;
		while (!done) {
			file_in.read(buffer, buffer_size);
			len = file_in.gcount();
			done = file_in.eof();

			if (XML_Parse(p, buffer, len, done) == XML_STATUS_ERROR) {
				std::ostringstream stream;
				stream << "XML: Parse error at line " << XML_GetCurrentLineNumber(p) << ": " << XML_ErrorString(XML_GetErrorCode(p));
				throw std::runtime_error(stream.str());
			}

			if (!header_written && cvat_xml.validate()) {
				file_out << cvat_xml.cvat_header(classes);
				header_written = true;
			}
		}

	} catch(std::exception &e) {
		retval = false;
		std::cerr << "ERROR: GMLConverter: " << e.what() << std::endl;
	}

	file_in.close();
	XML_ParserFree(p);

	file_out << cvat_xml.cvat_footer();
	file_out.close();

	return retval;
}
