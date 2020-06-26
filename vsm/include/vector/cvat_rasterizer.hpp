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

#pragma once

#include "raster/raster_image.hpp"

#include <expat.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>


class CVATPolygon {
public:
	CVATPolygon();
	~CVATPolygon();

	typedef enum class_value_t {
		CV_UNDEFINED = 0,
		CV_CLEAR = 1,
		CV_CLOUD_SHADOW = 2,
		CV_SEMI_TRANSPARENT_CLOUD = 3,
		CV_CLOUD = 4,

		CV_BACKGROUND = 1
	};

	int z_order;
	int occluded;
	int label_index;
	Magick::CoordinateList points;

	bool parse_points(const std::string &content);

	bool set_label(const std::string &label);

	void clear();
};

class CVATRasterizer {
public:
	CVATRasterizer();
	~CVATRasterizer();

	bool convert(const std::filesystem::path &path_in, const std::filesystem::path &path_out_nc, const std::filesystem::path &path_out_png);

	static void XMLCALL tag_start_handler(void *user_data, const XML_Char *el, const XML_Char **attr);
	static void XMLCALL tag_data_handler(void *user_data, const XML_Char *s, int len);
	static void XMLCALL tag_end_handler(void *user_data, const XML_Char *el);

	RasterImage image;

protected:
	bool rasterize();

	std::ifstream file_in;

	std::string task_name;
	std::string image_name;
	std::string last_tag, last_last_tag;
	CVATPolygon last_polygon;

	std::vector<CVATPolygon> polygons;
};

