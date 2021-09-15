//! @file
//! @brief ESA S2 Scene (Sen2Cor) Classification Map, in JP2 format
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

#include "raster/jp2_image.hpp"

#include <filesystem>

#define NUM_SCL_CLASSES	12	///< Number of classes in the Sen2Cor classification.


/**
 * @brief Class for an ESA Sentinel-2 Scene (Sen2Cor) Classification Map raster, in JP2 format.
 */
class ESA_S2_SCL_JP2_Image: public JP2_Image {
	public:
		/**
		 * Initialize an empty raster.
		 */
		ESA_S2_SCL_JP2_Image();

		/**
		 * De-initialize the raster.
		 */
		~ESA_S2_SCL_JP2_Image();

		/**
		 * @brief Sen2Cor classes.
		 * @note All other datasets would be mapped to the Sen2Cor classes. Then, from there on, the classes could be remapped to an application specific classification scheme.
		 */
		enum scl_class_t {
			SCL_NO_DATA = 0,	///< Missing pixel.
			SCL_SATURATED_OR_DEFECTIVE = 1,	///< Over-exposed or defective pixel.
			SCL_DARK_AREA_PIXELS = 2,	///< Under-exposed pixel.
			SCL_CLOUD_SHADOWS = 3,	///< Pixel shadowed by a cloud.
			SCL_VEGETATION = 4,	///< Pixel with vegetation.
			SCL_NOT_VEGETATED = 5,	///< Pixel without vegetation.
			SCL_WATER = 6,	///< Water pixel.
			SCL_UNCLASSIFIED = 7,	///< Not sure which class the pixel should belong to.
			SCL_CLOUD_MEDIUM_PROBABILITY = 8,	///< Cloud pixel with medium confidence.
			SCL_CLOUD_HIGH_PROBABILITY = 9,	///< Cloud pixel with high confidence.
			SCL_THIN_CIRRUS = 10,	///< Thin cirrus cloud pixel.
			SCL_SNOW = 11	///< Snow pixel.
		};

		static const std::string class_names[NUM_SCL_CLASSES];	///< List of class names.
		unsigned char class_map[NUM_SCL_CLASSES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};	///< No remapping between classes.
};

