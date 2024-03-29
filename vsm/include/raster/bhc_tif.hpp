//! @file
//! @brief Baetens & Hagolle classification map, in TIF format
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

#include "raster/tif_image.hpp"
#include <filesystem>


/**
 * @brief Class for Baetens & Hagolle classification map, in TIF format
 */
class BHC_TIF_Image: public TIF_Image {
	public:
		/**
		 * Initialize an empty raster.
		 */
		BHC_TIF_Image();

		/**
		 * De-initialize the raster.
		 */
		~BHC_TIF_Image();
};

