//! @file
//! @brief CVAT XML vector layer rasterization to PNG and NetCDF.
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


/**
 * @brief A class to parse a polygon from the CVAT XML format.
 */
class CVATPolygon {
public:
	/**
	 * Initialize an empty polygon parser.
	 */
	CVATPolygon();

	/**
	 * De-initialize the polygon parser.
	 */
	~CVATPolygon();

	/**
	 * @brief Classes used for KappaMask labels.
	 */
	enum class_value_t {
		CV_UNDEFINED = 0,	///< Exclude pixel from training, priority 0.
		CV_CLEAR = 1,	///< Clear pixel, priority -5.
		CV_CLOUD_SHADOW = 2,	///< Cloud shadow pixel, priority -4.
		CV_SEMI_TRANSPARENT_CLOUD = 3,	///< Cirrus pixel, priority -3.
		CV_CLOUD = 4,	///< Cumulus pixel, priority -2.
		CV_INVALID = 5,	///< Invalid pixel, priority -1.

		CV_COUNT = 6,	///< Number of classes.
		CV_BACKGROUND = 1	///< Default background class (clear, in this case).
	};

	/**
	 * @brief Render priority per class. Higher priority is rendered on top of lower priority.
	 * This serves as a work-around for CVAT versions without working Z-order support.
	 */
	static const int class_priority[CV_COUNT];

	int z_order;	///< Polygon depth order (does not work in early CVAT versions).
	int occluded;	///< Whether the polygon is occluded or not.
	int label_index;	///< Index of the class which this polygon belongs to. @sa class_value_t
	Magick::CoordinateList points;	///< List of GraphicsMagick++ pixel coordinates.

	/**
	 * @brief Assign a polygon.
	 * @param[in] poly Reference to the source polygon.
	 * @return A copy of the polygon.
	 */
	CVATPolygon &operator=(const CVATPolygon &poly);

	/**
	 * @brief Comparison operator for sorting polygons by their class_priority.
	 * @param[in] poly Reference to the other polygon to compare against.
	 * @return True if this polygon should be rendered below the other one.
	 */
	bool operator<(const CVATPolygon &poly) const;

	/**
	 * @brief Parse an XML string for polygon points.
	 * @param[in] content Reference to the XML string.
	 * @return True on success, false on failure.
	 */
	bool parse_points(const std::string &content);

	/**
	 * @brief Assign a label to the polygon.
	 * @param[in] label Reference to the name of the label.
	 * @return True.
	 */
	bool set_label(const std::string &label);

	/**
	 * @brief Clear the polygon and its points.
	 */
	void clear();
};

/**
 * @brief A rasterizer for the CVAT vector layer.
 */
class CVATRasterizer {
public:
	/**
	 * @brief Initialize an empty rasterizer.
	 */
	CVATRasterizer();

	/**
	 * @brief De-initialize the rasterizer.
	 */
	~CVATRasterizer();

	/**
	 * @brief Convert a CVAT vector layer into a raster file.
	 * @param[in] path_in Reference to the path to the input CVAT XML file.
	 * @param[in] path_out_nc Reference to the path to the output NetCDF file. Leave empty to skip NetCDF output.
	 * @param[in] path_out_png Reference to the path to the output PNG file. Leave empty to skip PNG output.
	 * @return True on success, otherwise false.
	 */
	bool convert(const std::filesystem::path &path_in, const std::filesystem::path &path_out_nc, const std::filesystem::path &path_out_png);

	/**
	 * @brief Expat callback for the start of an XML tag.
	 * @param[in] user_data Pointer to the CVATRasterizer class instance.
	 * @param[in] el Pointer to the name of the XML element.
	 * @param[in] attr Pointer to an array of element attributes.
	 */
	static void XMLCALL tag_start_handler(void *user_data, const XML_Char *el, const XML_Char **attr);

	/**
	 * @brief Expat callback for the content of an XML tag.
	 * @param[in] user_data Pointer to the CVATRasterizer class instance.
	 * @param[in] s Pointer to the element content.
	 * @param[in] len Content length in number of characters.
	 */
	static void XMLCALL tag_data_handler(void *user_data, const XML_Char *s, int len);

	/**
	 * @brief Expat callback for the end of an XML tag.
	 * @param[in] user_data Pointer to the CVATRasterizer class instance.
	 * @param[in] el Pointer to the name of the XML element.
	 */
	static void XMLCALL tag_end_handler(void *user_data, const XML_Char *el);

	RasterImage image;	///< Raster image instance.

protected:
	/**
	 * @brief Render the polygons onto the raster image.
	 * @return True on success, otherwise false.
	 */
	bool rasterize();

	std::ifstream file_in;	///< Input file stream.

	std::string task_name;	///< Name of the CVAT task.
	std::string image_name;	///< Name of the image which the vector mask corresponds to.
	std::string last_tag;	///< Name of the last XML tag.
	std::string last_last_tag;	///< Name of the prior to last XML tag.
	CVATPolygon last_polygon;	///< Last polygon parsed.

	std::vector<CVATPolygon> polygons;	///< Vector of polygons parsed.
};

