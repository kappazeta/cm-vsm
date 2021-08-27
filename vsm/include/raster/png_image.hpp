//! @file
//! @brief Tiled loading of a PNG image
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

#include <filesystem>


/**
 * @brief PNG raster image class.
 */
class PNG_Image: public RasterImage {
	public:
		/**
		 * Initialize an empty PNG raster.
		 */
		PNG_Image();

		/**
		 * De-initialize the raster.
		 */
		~PNG_Image();

		/**
		 * Load the image header.
		 * @param[in] path Reference to the PNG file path.
		 * @return True on success, false on failure.
		 */
		bool load_header(const std::filesystem::path &path);

		/**
		 * Load a subset of the PNG file.
		 * The top-left corner of the subset is specified by \f$x_0, y_0\f$
		 * and the bottom-right corner is specified by \f$x_1, y_1\f$.
		 * @param[in] path Reference to the PNG file path.
		 * @param da_x0 \f$x_0\f$ coordinate (left side) of the image to load.
		 * @param da_y0 \f$y_0\f$ coordinate (top side) of the image to load.
		 * @param da_x1 \f$x_1\f$ coordinate (right side) of the image to load.
		 * @param da_y1 \f$y_1\f$ coordinate (bottom side) of the image to load.
		 * @return True on success, False otherwise.
		 */
		bool load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1);
};

