//! @file
//! @brief Supervisely annotations format
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
 * @brief A class for an Supervise.ly image.
 */
class SuperviselyRaster: public RasterImage {
	public:
		/**
		 * Initialize an empty image.
		 */
		SuperviselyRaster();

		/**
		 * De-initialize the image.
		 */
		~SuperviselyRaster();

		/**
		 * Load a Supervise.ly image.
		 * @param[in] path_dir_in Reference to the path to the input directory which contains `/ds0/masks_machine/obj_class_to_machine_color.json`, etc.
		 * @param[in] product_tile_name Reference to the tile name, for which to load the mask raster from `/ds0/masks_machine/PRODUCT_TILE_NAME.png`.
		 * @return True on success, false on failure.
		 */
		bool load(const std::filesystem::path &path_dir_in, const std::string &product_tile_name);

		/**
		 * Convert the image into our format and our classification scheme.
		 * @param[in] path_dir Reference to the path to the input directory containing `supervisely_raster_PRODUCT_TILE_NAME.png`.
		 * @param[in] tile_name Reference to the `PRODUCT_TILE_NAME` to look for in the input directory.
		 * @param[in] path_nc Reference to the path to the NetCDF file to add the label to.
		 * @return True on success, false on failure.
		 */
		bool convert(const std::filesystem::path &path_dir, const std::string &tile_name, const std::filesystem::path &path_nc);
};
