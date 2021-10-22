//! @file
//! @brief Parser and converter for the Geography Markup Language (GML)
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


/**
 * @brief A GML feature (polygon) class.
 */
class GMLFeature {
public:
	/**
	 * @brief Initialize an empty feature.
	 */
	GMLFeature();

	/**
	 * @brief De-initialize the feature.
	 */
	~GMLFeature();

	//! \todo Support multiple polygons (with cut-outs).

	std::string fid;	///< Index of the feature.
	Polygon<float> coordinates;	///< Polygon coordinates.
	int dn;	///< Class index.
	bool inner_boundary;	///< True if the coordinates mark the inner boundary of the polygon.
};

/**
 * @brief GML to CVAT XML converter.
 * @note Assumes Flat Earth (only usable for small parcels where the curvature of the Earth can be ignored).
 */
class GMLConverter {
public:
	/**
	 * @brief Initialize the converter.
	 */
	GMLConverter();

	/**
	 * @brief De-initialize the converter.
	 */
	~GMLConverter();

	/**
	 * @brief Set class names and class indexes to include in the CVAT XML output.
	 * @param[in] Reference to a vector of class names to store in the CVAT XML header.
	 * @param[in] Reference to a vector of class indexes for the polygons to include in the CVAT XML.
	 */
	void set_classes(const std::vector<std::string> &classes, const std::vector<int> &include_classes);

	/**
	 * @brief Set task name, username, e-mail for the CVAT annotations.
	 * @param[in] task_name Reference to the name of the labelling task.
	 * @param[in] owner_username Reference to the username of the task owner.
	 * @param[in] owner_email Reference to the e-mail address of the task owner.
	 */
	void set_meta_info(const std::string &task_name, const std::string &owner_username, const std::string &owner_email);

	/**
	 * @brief Set a scaling factor for converting from geo-coordinates to pixel coordinates.
	 * @param f Scaling factor.
	 * @note Assumes that the curvature of the Earth can be ignored.
	 */
	void set_multiplier(float f);

	/**
	 * @brief Convert from GML to CVAT XML.
	 * @param[in] path_in Reference to the path to GML input.
	 * @param[in] path_out Reference to the path to CVAT XML output.
	 * @return True on success, otherwise false.
	 */
	bool convert(const std::string &path_in, const std::string &path_out);

	/**
	 * @brief Parse a polygon in GML.
	 * @param[in] content Reference to the XML string.
	 * @param[out] coordinates Pointer to the output polygon.
	 */
	void parse_polygon(const std::string &content, Polygon<float> *coordinates);

	/**
	 * @brief Expat callback for the start of an XML tag.
	 * @param[in] user_data Pointer to the GMLConverter class instance.
	 * @param[in] el Pointer to the name of the XML element.
	 * @param[in] attr Pointer to an array of element attributes.
	 */
	static void XMLCALL tag_start_handler(void *user_data, const XML_Char *el, const XML_Char **attr);

	/**
	 * @brief Expat callback for the content of an XML tag.
	 * @param[in] user_data Pointer to the GMLConverter class instance.
	 * @param[in] s Pointer to the element content.
	 * @param[in] len Content length in number of characters.
	 */
	static void XMLCALL tag_data_handler(void *user_data, const XML_Char *s, int len);

	/**
	 * @brief Expat callback for the end of an XML tag.
	 * @param[in] user_data Pointer to the GMLConverter class instance.
	 * @param[in] el Pointer to the name of the XML element.
	 */
	static void XMLCALL tag_end_handler(void *user_data, const XML_Char *el);

	std::vector<GMLFeature *> features;	///< Vector of pointers to GML features.
	GMLFeature *last_feature;	///< Pointer to the last parsed GML feature.
	std::string last_tag;	///< Name of the last XML tag in GML.

	CVATXML cvat_xml;	///< A CVAT XML generator instance.

protected:
	float multiplier;	///< Multiplier for converting coordinates.

	std::ifstream file_in;	///< Input file stream.
	std::ofstream file_out;	///< Output file stream.

	std::vector<std::string> classes;	///< Class names used in the GML, to be stored in CVAT XML header.
	std::vector<int> include_classes;	///< Indexes of the classes to be included in the CVAT XML.
};

