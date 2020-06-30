// CVAT XML vector layer rasterization to PNG and NetCDF.
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

#include "vector/cvat_rasterizer.hpp"

#include <string.h>


const int CVATPolygon::class_priority[CVATPolygon::CV_COUNT] = {
	0, -4, -3, -2, -1
};

CVATPolygon::CVATPolygon(): z_order(0), occluded(0), label_index(0) {}

CVATPolygon::~CVATPolygon() {
	clear();
}

void CVATPolygon::clear() {
	points.clear();
}

CVATPolygon &CVATPolygon::operator=(const CVATPolygon &poly) {
	z_order = poly.z_order;
	occluded = poly.occluded;
	label_index = poly.label_index;
	points = poly.points;

	return *this;
}

bool CVATPolygon::operator<(const CVATPolygon &poly) const {
	if (label_index < CV_COUNT && poly.label_index < CV_COUNT)
		return (class_priority[label_index] < class_priority[poly.label_index]);
	return false;
}

bool CVATPolygon::parse_points(const std::string &content) {
	size_t idx_sep_start = 0;
	size_t idx_sep_end = 0;
	float x, y;

	std::istringstream stream(content);
	std::string token, x_str, y_str;

	while (std::getline(stream, token, ';')) {
		idx_sep_start = token.find_first_of(",");
		idx_sep_end = token.find_last_of(",");

		if (idx_sep_start == std::string::npos || idx_sep_end == std::string::npos)
			break;

		x_str = token.substr(0, idx_sep_start);
		y_str = token.substr(idx_sep_end + 1);
		x = atof(x_str.c_str());
		y = atof(y_str.c_str());

		points.push_back(Magick::Coordinate(x, y));
	}

	return true;
}

bool CVATPolygon::set_label(const std::string &label) {
	label_index = 0;

	if (label == "CLEAR")
		label_index = CV_CLEAR;
	else if (label == "CLOUD")
		label_index = CV_CLOUD;
	else if (label == "SEMI_TRANSPARENT_CLOUD")
		label_index = CV_SEMI_TRANSPARENT_CLOUD;
	else if (label == "CLOUD_SHADOW")
		label_index = CV_CLOUD_SHADOW;
	return true;
}

CVATRasterizer::CVATRasterizer() {}

CVATRasterizer::~CVATRasterizer() {
	image.clear();
}

void XMLCALL CVATRasterizer::tag_start_handler(void *user_data, const XML_Char *el, const XML_Char **attr) {
	CVATRasterizer *p_inst = (CVATRasterizer *) user_data;

	if (!strncmp(el, "image", 5)) {
		for (int i=0; attr[i]; i+=2) {
			if (!strncmp(attr[i], "name", 4))
				p_inst->image_name.assign(attr[i + 1]);
			else if (!strncmp(attr[i], "width", 5))
				p_inst->image.main_geometry.width(atoi(attr[i + 1]));
			else if (!strncmp(attr[i], "height", 6))
				p_inst->image.main_geometry.height(atoi(attr[i + 1]));
		}
	} else if (!strncmp(el, "polygon", 7)) {
		for (int i=0; attr[i]; i+=2) {
			if (!strncmp(attr[i], "label", 5))
				p_inst->last_polygon.set_label(attr[i + 1]);
			else if (!strncmp(attr[i], "occluded", 8))
				p_inst->last_polygon.occluded = atoi(attr[i + 1]);
			else if (!strncmp(attr[i], "z_order", 7))
				p_inst->last_polygon.z_order = atoi(attr[i + 1]);
			else if (!strncmp(attr[i], "points", 6))
				p_inst->last_polygon.parse_points(attr[i + 1]);
		}
	}

	p_inst->last_last_tag.assign(p_inst->last_tag);
	p_inst->last_tag.assign(el);
}

void XMLCALL CVATRasterizer::tag_data_handler(void *user_data, const XML_Char *s, int len) {
	CVATRasterizer *p_inst = (CVATRasterizer *) user_data;
	std::string content(s, len);

	if (p_inst->last_last_tag == "task" && p_inst->last_tag == "name") {
		p_inst->task_name = content;
	}
}

void XMLCALL CVATRasterizer::tag_end_handler(void *user_data, const XML_Char *el) {
	CVATRasterizer *p_inst = (CVATRasterizer *) user_data;

	if (!strncmp(el, "polygon", 7)) {
		p_inst->polygons.push_back(p_inst->last_polygon);
		p_inst->last_polygon.clear();
	}

	p_inst->last_tag.assign("");
}

bool CVATRasterizer::convert(const std::filesystem::path &path_in, const std::filesystem::path &path_out_nc, const std::filesystem::path &path_out_png) {
	XML_Parser p = nullptr;
	bool retval = true;
	const int buffer_size = 100 * 1024;
	char buffer[buffer_size];

	try {
		file_in.open(path_in, std::ios_base::in);

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
		}

		rasterize();

		if (path_out_png.string().length() > 0)
			retval &= image.save(path_out_png);
		if (path_out_nc.string().length() > 0)
			retval &= image.add_to_netcdf(path_out_nc, "Label", 9);

	} catch(std::exception &e) {
		retval = false;
		std::cerr << "ERROR: CVATRasterizer: " << e.what() << std::endl;
	}

	file_in.close();
	XML_ParserFree(p);

	return retval;
}

bool CVATRasterizer::rasterize() {
	image.create_grayscale(image.main_geometry, 4, CVATPolygon::CV_BACKGROUND);

	std::list<Magick::Drawable> drawlist;

	std::sort(polygons.begin(), polygons.end());

	double color;
	for (int i=0; i<polygons.size(); i++) {
		color = polygons[i].label_index / 255.0;

		drawlist.push_back(Magick::DrawableStrokeAntialias(false));
		drawlist.push_back(Magick::DrawableStrokeColor(Magick::ColorGray(color)));
		drawlist.push_back(Magick::DrawableFillColor(Magick::ColorGray(color)));
		drawlist.push_back(Magick::DrawablePolygon(polygons[i].points));
	}

	image.subset->draw(drawlist);
	return true;
}

