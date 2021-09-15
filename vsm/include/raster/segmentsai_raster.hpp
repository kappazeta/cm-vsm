//! @file
//! @brief Segments.AI annotations format
//
// Copyright 2021 KappaZeta Ltd.
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
 * @brief A Segments.AI raster class.
 */
class SegmentsAIRaster: public RasterImage {
	public:
		/**
		 * Initialize an empty raster.
		 */
		SegmentsAIRaster();

		/**
		 * De-initialize the raster.
		 */
		~SegmentsAIRaster();

		/**
		 * Load the raster.
		 * @param[in] mask_path Reference to the path to the mask image.
		 * @param[in] classes_path Reference to the path of the metadata file with classes and the pixel values which they correspond to.
		 * @return True on success, otherwise false.
		 */
		bool load(const std::filesystem::path &mask_path, const std::filesystem::path &classes_path);

		/**
		 * Convert the loaded raster into our own format (PNG and NetCDF) and classification scheme.
		 * @param[in] path_dir Reference to the path to the directory to store the PNG and NetCDF files in. If the NetCDF file already exists, then the `Label` channel is replaced.
		 * @return True on success, otherwise false.
		 */
		bool convert(const std::filesystem::path &path_dir);
};

