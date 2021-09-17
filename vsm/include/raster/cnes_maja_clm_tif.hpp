//! @file
//! @brief CNES MAJA Classification Map, in TIF format
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
 * @brief Class for CNES MAJA Classification Map, in TIF format.
 */
class CNES_MAJA_CLM_TIF: public TIF_Image {
	public:
		/**
		 * Initialize an empty raster.
		 */
		CNES_MAJA_CLM_TIF();

		/**
		 * De-initialize the raster.
		 */
		~CNES_MAJA_CLM_TIF();

		/**
		 * @brief MAJA classification flags.
		 * \par For reference
		 * https://labo.obs-mip.fr/multitemp/sentinel-2/majas-native-sentinel-2-format/#English
		 */
		enum clm_flag_t {
			CLM_CLOUDS_SHADOWS = 1,         ///< All clouds except the thinnest and all shadows.
			CLM_CLOUDS = 2,                 ///< All clouds (except the thinnest).
			CLM_CLOUD_SHADOWS = 4,          ///< Cloud shadows cast by a detected cloud.
			CLM_CLOUD_SHADOWS_OUTSIDE = 8,  ///< Cloud shadows cast by a cloud outside image.
			CLM_CLOUDS_MONOTEMP = 16,       ///< Clouds detected via mono-temporal thresholds.
			CLM_CLOUDS_MULTITEMP = 32,      ///< Clouds detected via multi-temporal thresholds.
			CLM_THIN_CLOUDS = 64,           ///< Thinnest clouds.
			CLM_HIGH_CLOUDS = 128           ///< High clouds detected by 1.38 µm.
		};

		/**
		 * Remap pixel values from MAJA cloud mask into Sen2Cor format.
		 */
		static RasterImage *remap_majac_values(RasterImage *img);
};

