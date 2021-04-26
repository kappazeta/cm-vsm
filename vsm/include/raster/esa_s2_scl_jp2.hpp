// ESA S2 Scene Classification Map, in JP2 format
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

#define NUM_SCL_CLASSES	12


class ESA_S2_SCL_JP2_Image: public JP2_Image {
	public:
		ESA_S2_SCL_JP2_Image();
		~ESA_S2_SCL_JP2_Image();

		enum scl_class_t {
			SCL_NO_DATA = 0,
			SCL_SATURATED_OR_DEFECTIVE = 1,
			SCL_DARK_AREA_PIXELS = 2,
			SCL_CLOUD_SHADOWS = 3,
			SCL_VEGETATION = 4,
			SCL_NOT_VEGETATED = 5,
			SCL_WATER = 6,
			SCL_UNCLASSIFIED = 7,
			SCL_CLOUD_MEDIUM_PROBABILITY = 8,
			SCL_CLOUD_HIGH_PROBABILITY = 9,
			SCL_THIN_CIRRUS = 10,
			SCL_SNOW = 11
		};

		static const std::string class_names[NUM_SCL_CLASSES];
		unsigned char class_map[NUM_SCL_CLASSES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
};

