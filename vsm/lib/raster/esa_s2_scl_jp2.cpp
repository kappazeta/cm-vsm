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

#include "raster/esa_s2_scl_jp2.hpp"


ESA_S2_SCL_JP2_Image::ESA_S2_SCL_JP2_Image() {}
ESA_S2_SCL_JP2_Image::~ESA_S2_SCL_JP2_Image() {}


// Used as reference:
//  https://sentinel.esa.int/web/sentinel/technical-guides/sentinel-2-msi/level-2a/algorithm
const std::string ESA_S2_SCL_JP2_Image::class_names[] = {
	"NO_DATA",                  // 0
	"SATURATED_OR_DEFECTIVE",   // 1
	"DARK_AREA_PIXELS",         // 2
	"CLOUD_SHADOWS",            // 3
	"VEGETATION",               // 4
	"NOT_VEGETATED",            // 5
	"WATER",                    // 6
	"UNCLASSIFIED",             // 7
	"CLOUD_MEDIUM_PROBABILITY", // 8
	"CLOUD_HIGH_PROBABILITY",   // 9
	"THIN_CIRRUS",              // 10
	"SNOW"                      // 11
};

