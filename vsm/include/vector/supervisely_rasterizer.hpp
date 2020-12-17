// Supervise.ly JSON vector layer rasterization to PNG and NetCDF.
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

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>


class SuperviselyPolygon {
public:
	SuperviselyPolygon();
	~SuperviselyPolygon();

	typedef enum class_value_t {
		SVLY_UNDEFINED = 0,	///< Exclude pixel from training, priority 0.
		SVLY_CLEAR = 1,	///< Clear pixel, priority -4.
		SVLY_CLOUD_SHADOW = 2,	///< Cloud shadow pixel, priority -3.
		SVLY_SEMI_TRANSPARENT_CLOUD = 3,	///< Cirrus pixel, priority -2.
		SVLY_CLOUD = 4,	///< Cumulus pixel, priority -1.

		SVLY_COUNT = 5,	///< Number of classes.
		SVLY_BACKGROUND = 1	///< Default background class (clear, in this case).
	};

	//! Render priority per class. Higher priority is rendered on top of lower priority.
	static const int class_priority[SVLY_COUNT];

	int label_index;
	Magick::CoordinateList exterior;
	Magick::CoordinateList interior;

	SuperviselyPolygon &operator=(const SuperviselyPolygon &poly);
	bool operator<(const SuperviselyPolygon &poly) const;

	bool parse_points(const nlohmann::json &j);

	bool set_label(const std::string &label);

	void clear();
};

class SuperviselyRasterizer {
public:
	SuperviselyRasterizer();
	~SuperviselyRasterizer();

	bool convert(const std::filesystem::path &path_dir_in, const std::string &product_tile_name, const std::filesystem::path &path_out_nc, const std::filesystem::path &path_out_png);

	RasterImage image;

protected:
	bool rasterize();

	std::ifstream file_in;

	std::string description;

	std::vector<SuperviselyPolygon> polygons;
};

