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

#include "vector/cvat.hpp"

#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>


CVATXML::CVATXML(): w(0), h(0) {}
CVATXML::~CVATXML() {}

std::string CVATXML::cvat_header(const std::vector<std::string> &classes) {
	std::ostringstream ss;

	std::time_t ct = std::time(0);
	char ctstr[100];
	std::strftime(ctstr, sizeof(ctstr), "%Y-%m-%d %H:%M:%S", std::localtime(&ct));

	this->classes = classes;

	ss << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		<< "<annotations>\n"
		<< "  <version>1.1</version>\n"
		<< "  <meta>\n"
		<< "    <task>\n"
		<< "      <id>0</id>\n"
		<< "      <name>" << task_name << "</name>\n"
		<< "      <size>1</size>\n"
		<< "      <mode>annotation</mode>\n"
		<< "      <overlap>0</overlap>\n"
		<< "      <bugtracker></bugtracker>\n"
		<< "      <created>" << ctstr << "</created>\n"
		<< "      <updated>" << ctstr << "</updated>\n"
		<< "      <start_frame>0</start_frame>\n"
		<< "      <stop_frame>0</stop_frame>\n"
		<< "      <frame_filter></frame_filter>\n"
		<< "      <z_order>False</z_order>\n"
		<< "      <labels>\n";

	for (size_t i=0; i<classes.size(); i++) {
		ss << "        <label>\n"
			<< "          <name>" << classes[i] << "</name>\n"
			<< "          <attributes></attributes>\n"
			<< "        </label>\n";
	}

	ss << "      </labels>\n"
		<< "      <segments></segments>\n";

	ss << "      <owner>\n"
		<< "        <username>" << owner_username << "</username>\n"
		<< "        <email>" << owner_email << "</email>\n"
		<< "      </owner>\n"
		<< "      <assignee></assignee>\n"
		<< "    </task>\n"
		<< "    <dumped>" << ctstr << "</dumped>\n"
		<< "  </meta>\n"
		<< "  <image id=\"0\" name=\"" << filename << "\" width=\"" << w << "\" height=\"" << h << "\">\n";

	return ss.str();
}

std::string CVATXML::cvat_polygon(unsigned int class_index, const Polygon<float> &coordinates) {
	std::ostringstream ss;

	if (class_index >= classes.size())
		return "";

	//! \todo Polygon class with proper string representation methods.

	if (coordinates.size() < 3) {
		std::cerr << "WARN: CVATXML: Polygon of class " << class_index
			<< " only has " << coordinates.size() << " vertices, skipping." << std::endl;
		return "";
	}

	ss << "    <polygon label=\"" << classes[class_index] << "\" occluded=\"0\" points=\"";
	ss << std::fixed << std::setprecision(2);

	for (size_t i=0; i<coordinates.size(); i++) {
		ss << coordinates[i].x << "," << coordinates[i].y;
		if (i < coordinates.size() - 1)
			ss << ";";
	}

	ss << "\"></polygon>\n";

	return ss.str();
}

std::string CVATXML::cvat_footer() {
	std::ostringstream ss;

	ss << "  </image>\n"
		<< "</annotations>\n";

	return ss.str();
}

bool CVATXML::validate() {
	if (w > 0 && h > 0 && filename.length() > 0)
		return true;
	return false;
}

