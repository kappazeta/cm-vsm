// CNES MAJA Cloud Mask format
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

#include "raster/cnes_maja_clm_tif.hpp"
#include "raster/esa_s2_scl_jp2.hpp"

CNES_MAJA_CLM_TIF::CNES_MAJA_CLM_TIF() {}
CNES_MAJA_CLM_TIF::~CNES_MAJA_CLM_TIF() {}


RasterImage *CNES_MAJA_CLM_TIF::remap_majac_values(RasterImage *img) {
	const float v_cloud = ESA_S2_SCL_JP2_Image::SCL_CLOUD_HIGH_PROBABILITY / 255.0f;
	const float v_shadow = ESA_S2_SCL_JP2_Image::SCL_CLOUD_SHADOWS / 255.0f;
	const float v_cirrus = ESA_S2_SCL_JP2_Image::SCL_THIN_CIRRUS / 255.0f;
	const float v_clear = ESA_S2_SCL_JP2_Image::SCL_VEGETATION / 255.0f;

	if (img != nullptr && img->subset != nullptr) {
		unsigned int w = img->subset->columns();
		unsigned int h = img->subset->rows();
		unsigned int size = w * h;

		Magick::PixelPacket *px = img->subset->getPixels(0, 0, w, h);

		float src_val, dst_val;

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(px[i]).shade();

			unsigned char value = (unsigned char) (255 * src_val);

			if ((value & CLM_CLOUDS) != 0)
				dst_val = v_cloud;
			else if ((value & (CLM_CLOUD_SHADOWS | CLM_CLOUD_SHADOWS_OUTSIDE)) != 0)
				dst_val = v_shadow;
			else if ((value & CLM_THIN_CLOUDS) != 0)
				dst_val = v_cirrus;
			else
				dst_val = v_clear;

			px[i] = Magick::ColorGray(dst_val);
		}

		img->subset->syncPixels();
	}

	return img;
}

