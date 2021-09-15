//! @file
//! @brief Supervise.ly JSON vector layer rasterization to PNG and NetCDF.
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


/**
 * @brief Supervise.ly polygon parser.
 */
class SuperviselyPolygon {
public:
	/**
	 * @brief Initialize the parser.
	 */
	SuperviselyPolygon();

	/**
	 * De-initialize the parser.
	 */
	~SuperviselyPolygon();

	/**
	 * @brief Classes used for KappaMask labelling.
	 */
	enum class_value_t {
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

	int label_index;	///< Label index of the polygon.
	Magick::CoordinateList exterior;	///< Line of vertices marking the exterior of the polygon.
	Magick::CoordinateList interior;	///< Line of vertices marking the interior of the polygon.

	/**
	 * @brief Assign a polygon parser.
	 * @param[in] poly Reference to the source polygon parser instance.
	 * @return A copy of the polygon parser.
	 */
	SuperviselyPolygon &operator=(const SuperviselyPolygon &poly);

	/**
	 * @brief Compare polygons by their priority.
	 * @param[in] poly Reference to the other polygon to compare against.
	 * @return True if this polygon is to be rendered below the other one.
	 */
	bool operator<(const SuperviselyPolygon &poly) const;

	/**
	 * @brief Parse vertex coordinates from a JSON dict.
	 * @param[in] j Reference to the JSON dict to be parsed.
	 * @return True on success, otherwise false.
	 */
	bool parse_points(const nlohmann::json &j);

	/**
	 * @brief Assign a label to this polygon.
	 * @param[in] label Reference to the name of the class to assign to this polygon.
	 * @return True on success, otherwise false.
	 */
	bool set_label(const std::string &label);

	/**
	 * @brief Clear the polygon with all its coordinates.
	 */
	void clear();
};

/**
 * @brief A Supervise.ly rasterizer class.
 * https://supervise.ly/
 */
class SuperviselyRasterizer {
public:
	/**
	 * @brief Initialize the rasterizer.
	 */
	SuperviselyRasterizer();

	/**
	 * @brief De-initialize the rasterizer.
	 */
	~SuperviselyRasterizer();

	/**
	 * @brief Convert from Supervise.ly format into a raster with the KappaMask classification scheme.
	 * @param[in] path_dir_in Reference to the path to the directory which contains `/ds0/ann/PRODUCT_TILE_NAME.png.json`.
	 * @param[in] product_tile_name Reference to the name of the product tile to convert.
	 * @param[in] path_out_nc Reference to the path to the output NetCDF file. Leave empty to skip NetCDF output.
	 * @param[in] path_out_png Reference to the path to the output PNG file. Leave empty to skip PNG output.
	 * @return True on success, otherwise false.
	 */
	bool convert(const std::filesystem::path &path_dir_in, const std::string &product_tile_name, const std::filesystem::path &path_out_nc, const std::filesystem::path &path_out_png);

	RasterImage image;	///< Output raster instance.

protected:
	/**
	 * @brief Rasterize the vector input.
	 */
	bool rasterize();

	std::ifstream file_in;	///< Input file stream.

	std::string description;	///< Description from the Supervise.ly metadata.

	std::vector<SuperviselyPolygon> polygons;	///< Vector of parsed polygons.
};

