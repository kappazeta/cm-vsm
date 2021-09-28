// Processing of ESA S2 L2A products
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

#include "raster/esa_s2.hpp"
#include "raster/esa_s2_scl_jp2.hpp"
#include "raster/esa_s2_band_jp2.hpp"
#include "raster/bhc_tif.hpp"
#include "raster/cnes_maja_clm_tif.hpp"
#include "raster/png_image.hpp"

#include "util/text.hpp"
#include <algorithm>
#include <math.h>
#include <set>

// GDAL
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>


const std::string ESA_S2_Image_Operator::data_type_name[ESA_S2_Image_Operator::DT_COUNT] = {
	"TCI", "SCL", "AOT", "B01", "B02", "B03", "B04", "B05", "B06", "B07", "B08", "B8A", "B09", "B10", "B11", "B12",	"WVP",
	"GML", "S2CC", "S2CS", "FMC", "SS2C", "SS2CC", "MAJAC", "BHC", "FMSC", "GSFC", "DL-L8S2-UV"
};

//! Map BHC classes to SCL classes.
const unsigned char ESA_S2_Image_Operator::bhc_scl_value_map[9] = {
	0,  // 0  NO_DATA                  -> NO_DATA
	0,  // 1  NOT_USED                 -> NO_DATA
	8,  // 2  LOW_CLOUDS               -> CLOUD_MEDIUM_PROBABILITY
	9,  // 3  HIGH_CLOUDS              -> CLOUD_HIGH_PROBABILITY
	3,  // 4  CLOUD_SHADOWS            -> CLOUD_SHADOWS
	4,  // 5  LAND                     -> VEGETATION
	6,  // 6  WATER                    -> WATER
	11, // 7  SNOW                     -> SNOW
	0   // 8 - 255                     -> NO_DATA
};

//! Map FMC classes to SCL classes.
const unsigned char ESA_S2_Image_Operator::fmc_scl_value_map[6] = {
	4,  // 0  CLEAR                    -> VEGETATION
	6,  // 1  WATER                    -> WATER
	3,  // 2  CLOUD_SHADOWS            -> CLOUD_SHADOWS
	11, // 3  SNOW                     -> SNOW
	9,  // 4  CLOUD                    -> CLOUD_HIGH_PROBABILITY
	0   // 5 - 255                     -> NO_DATA
};

//! Map SS2C classes to SCL classes.
const unsigned char ESA_S2_Image_Operator::ss2c_scl_value_map[3] = {
	4,  // 0  CLEAR                    -> VEGETATION
	9,  // 1  CLOUD                    -> CLOUD_HIGH_PROBABILITY
	0   // 2 - 255                     -> NO_DATA
};

//! Map FMSC classes to SCL classes.
const unsigned char ESA_S2_Image_Operator::fmsc_scl_value_map[4] = {
	4,  // 0  CLEAR                    -> VEGETATION
	9,  // 1  CLOUD                    -> CLOUD_HIGH_PROBABILITY
	3,  // 2  CLOUD_SHADOWS            -> CLOUD_SHADOWS
	0   // 2 - 255                     -> NO_DATA
};

//! Map GSFC classes to SCL classes.
const unsigned char ESA_S2_Image_Operator::gsfc_scl_value_map[6] = {
	7,  // 0  UNCLASSIFIED             -> UNCLASSIFIED
	4,  // 1  CLEAR                    -> VEGETATION
	9,  // 2  CLOUD                    -> CLOUD_HIGH_PROBABILITY
	10, // 3  CIRRUS_CLOUD             -> THIN_CIRRUS
	3,  // 4  CLOUD_SHADOWS            -> CLOUD_SHADOWS
	0   // 5 - 255                     -> NO_DATA
};

//! Map DL-L8S2-UV classes to SCL classes.
const unsigned char ESA_S2_Image_Operator::dl_l8s2_uv_scl_value_map[4] = {
	0,  // 0                           -> NO_DATA
	4,  // 1  CLEAR                    -> VEGETATION
	9,  // 2  CLOUD                    -> CLOUD_HIGH_PROBABILITY
	0   // 3 - 255                     -> NO_DATA
};

ESA_S2_Image::ESA_S2_Image():
	tile_size(512), scl_value_map(nullptr), max_scl_value(12), f_downscale(1), f_overlap(0.0f),
	store_png(false), read_tiled(false), num_threads(0), geo_extracted(false) {
}
ESA_S2_Image::~ESA_S2_Image() {}

void ESA_S2_Image::set_tile_size(int tile_size) {
	this->tile_size = tile_size;
}

void ESA_S2_Image::set_scl_class_map(unsigned char *class_map) {
	scl_value_map = class_map;
}

void ESA_S2_Image::set_downscale_factor(int f) {
	if (f <= 0)
		f_downscale = 1;
	else
		f_downscale = f;
}

void ESA_S2_Image::set_deflate_factor(int d) {
	deflate_factor = d;
}

void ESA_S2_Image::set_overlap_factor(float f) {
	if (f <= 0.0f)
		f_overlap = 0.0f;
	else if (f >= 0.5)
		f_overlap = 0.5f;
	else
		f_overlap = f;
}

void ESA_S2_Image::set_resampling_method(const std::string &m) {
	resampling_method_name = m;
}

void ESA_S2_Image::set_png_output(bool enabled) {
	store_png = enabled;
}

void ESA_S2_Image::set_tiled_input(bool enabled) {
	read_tiled = enabled;
}

void ESA_S2_Image::set_num_threads(int num_threads) {
	this->num_threads = num_threads;
}

void ESA_S2_Image::set_aoi_geometry(const std::string &wkt_geom) {
	wkt_geom_aoi = wkt_geom;
}

std::string ESA_S2_Image::get_product_name_from_path(const std::filesystem::path &path) {
	for (auto it = path.begin(); it != path.end(); ++it) {
		if (endswith(*it, ".SAFE"))
			return *it;
	}
	return "";
}

bool ESA_S2_Image::process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, std::vector<std::string> bands) {
	ESA_S2_Image_Operator::data_resolution_t data_resolution;
	std::vector<bool> b(ESA_S2_Image_Operator::DT_COUNT, false);

	// Build a vector of booleans to indicate the bands to be processed.
	for (std::vector<std::string>::iterator it = bands.begin(); it != bands.end(); it++) {
		for (int i=0; i<ESA_S2_Image_Operator::DT_COUNT; i++) {
			if (*it == ESA_S2_Image_Operator::data_type_name[i]) {
				b[i] = true;
			}
		}
	}

	// TODO:: Generalize the following.

	for (const auto &granule_entry: std::filesystem::directory_iterator(path_dir_in.string() + "/GRANULE/")) {
		data_resolution = ESA_S2_Image_Operator::DR_10M;
		// Files within an L2A product with a 10 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/IMG_DATA/R10m/")) {
			for (const auto &r10m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/R10m/")) {
				//! \todo Map these
				if (endswith(r10m_entry.path().string(), "_TCI_10m.jp2") && b[ESA_S2_Image_Operator::DT_TCI]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_TCI, data_resolution);
				} else if (endswith(r10m_entry.path().string(), "_AOT_10m.jp2") && b[ESA_S2_Image_Operator::DT_AOT]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_AOT, data_resolution);
				} else if (endswith(r10m_entry.path().string(), "_WVP_10m.jp2") && b[ESA_S2_Image_Operator::DT_WVP]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_WVP, data_resolution);
				} else if (endswith(r10m_entry.path().string(), "_B02_10m.jp2") && b[ESA_S2_Image_Operator::DT_B02]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B02, data_resolution);
				} else if (endswith(r10m_entry.path().string(), "_B03_10m.jp2") && b[ESA_S2_Image_Operator::DT_B03]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B03, data_resolution);
				} else if (endswith(r10m_entry.path().string(), "_B04_10m.jp2") && b[ESA_S2_Image_Operator::DT_B04]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B04, data_resolution);
				} else if (endswith(r10m_entry.path().string(), "_B08_10m.jp2") && b[ESA_S2_Image_Operator::DT_B08]) {
					splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B08, data_resolution);
				}
			}
		}
		// Files within an L1C product with a 10 m resolution.
		for (const auto &r10m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/")) {
			//! \todo Map these
			if (endswith(r10m_entry.path().string(), "_TCI.jp2") && b[ESA_S2_Image_Operator::DT_TCI]) {
				splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_TCI, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B02.jp2") && b[ESA_S2_Image_Operator::DT_B02]) {
				splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B02, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B03.jp2") && b[ESA_S2_Image_Operator::DT_B03]) {
				splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B03, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B04.jp2") && b[ESA_S2_Image_Operator::DT_B04]) {
				splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B04, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B08.jp2") && b[ESA_S2_Image_Operator::DT_B08]) {
				splitJP2(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B08, data_resolution);
			}
		}
		// Sinergise's S2Cloudless classification map within an L1C product, with a 10 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/S2CLOUDLESS_DATA/R10m/")) {
			for (const auto &s2c_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/S2CLOUDLESS_DATA/R10m/")) {
				if (endswith(s2c_entry.path().string(), "_prediction.png") && b[ESA_S2_Image_Operator::DT_SS2C]) {
					splitPNG(s2c_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2C, data_resolution);
				} else if (endswith(s2c_entry.path().string(), "_probability.png") && b[ESA_S2_Image_Operator::DT_SS2CC]) {
					splitPNG(s2c_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2CC, data_resolution);
				}
			}
		}
		// CNES MAJA classification map within an L1C / L2A product, with a 10 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/MAJA_DATA/")) {
			for (const auto &majac_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/MAJA_DATA/")) {
				if (endswith(majac_entry.path().string(), "_CLM_R1.tif") && b[ESA_S2_Image_Operator::DT_MAJAC]) {
					splitTIF(majac_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_MAJAC, data_resolution);
				}
			}
		}

		data_resolution = ESA_S2_Image_Operator::DR_20M;
		// Files within an L2A product with a 20 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/IMG_DATA/R20m/")) {
			for (const auto &r20m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/R20m/")) {
				if (endswith(r20m_entry.path().string(), "_SCL_20m.jp2") && b[ESA_S2_Image_Operator::DT_SCL]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SCL, data_resolution);
				} else if (endswith(r20m_entry.path().string(), "_B05_20m.jp2") && b[ESA_S2_Image_Operator::DT_B05]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B05, data_resolution);
				} else if (endswith(r20m_entry.path().string(), "_B06_20m.jp2") && b[ESA_S2_Image_Operator::DT_B06]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B06, data_resolution);
				} else if (endswith(r20m_entry.path().string(), "_B07_20m.jp2") && b[ESA_S2_Image_Operator::DT_B07]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B07, data_resolution);
				} else if (endswith(r20m_entry.path().string(), "_B8A_20m.jp2") && b[ESA_S2_Image_Operator::DT_B8A]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B8A, data_resolution);
				} else if (endswith(r20m_entry.path().string(), "_B11_20m.jp2") && b[ESA_S2_Image_Operator::DT_B11]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B11, data_resolution);
				} else if (endswith(r20m_entry.path().string(), "_B12_20m.jp2") && b[ESA_S2_Image_Operator::DT_B12]) {
					splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B12, data_resolution);
				}
			}
		}
		// Files within an L1C product with a 20 m resolution.
		for (const auto &r20m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/")) {
			if (endswith(r20m_entry.path().string(), "_B05.jp2") && b[ESA_S2_Image_Operator::DT_B05]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B05, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B06.jp2") && b[ESA_S2_Image_Operator::DT_B06]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B06, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B07.jp2") && b[ESA_S2_Image_Operator::DT_B07]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B07, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B8A.jp2") && b[ESA_S2_Image_Operator::DT_B8A]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B8A, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B11.jp2") && b[ESA_S2_Image_Operator::DT_B11]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B11, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B12.jp2") && b[ESA_S2_Image_Operator::DT_B12]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B12, data_resolution);
			}
		}
		// Split Sen2cor cloudmask probabilities.
		for (const auto &r20m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/QI_DATA")) {
			if (endswith(r20m_entry.path().string(), "MSK_CLDPRB_20m.jp2") && b[ESA_S2_Image_Operator::DT_S2CC]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_S2CC, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "MSK_SNWPRB_20m.jp2") && b[ESA_S2_Image_Operator::DT_S2CS]) {
				splitJP2(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_S2CS, data_resolution);
			}
		}
		// Fmask4 classification map within an L1C product, with a 20 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/FMASK_DATA/")) {
			for (const auto &fmask_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/FMASK_DATA/")) {
				if (endswith(fmask_entry.path().string(), "_Fmask4.tif") && b[ESA_S2_Image_Operator::DT_FMC]) {
					splitTIF(fmask_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_FMC, data_resolution);
				}
			}
		}
		// Sinergise's S2Cloudless classification map within an L1C product, with a 20 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/S2CLOUDLESS_DATA/R20m/")) {
			for (const auto &s2c_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/S2CLOUDLESS_DATA/R20m/")) {
				if (endswith(s2c_entry.path().string(), "_prediction.png") && b[ESA_S2_Image_Operator::DT_SS2C]) {
					splitPNG(s2c_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2C, data_resolution);
				} else if (endswith(s2c_entry.path().string(), "_probability.png") && b[ESA_S2_Image_Operator::DT_SS2CC]) {
					splitPNG(s2c_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2CC, data_resolution);
				}
			}
		}
		// CNES MAJA classification map within an L1C / L2A product, with a 20 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/MAJA_DATA/")) {
			for (const auto &majac_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/MAJA_DATA/")) {
				if (endswith(majac_entry.path().string(), "_CLM_R2.tif") && b[ESA_S2_Image_Operator::DT_MAJAC]) {
					splitTIF(majac_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_MAJAC, data_resolution);
				}
			}
		}

		data_resolution = ESA_S2_Image_Operator::DR_60M;
		// Files within an L2A product with a 60 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/IMG_DATA/R60m/")) {
			for (const auto &r60m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/R60m/")) {
				if (endswith(r60m_entry.path().string(), "_B01_60m.jp2") && b[ESA_S2_Image_Operator::DT_B01]) {
					splitJP2(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B01, data_resolution);
				} else if (endswith(r60m_entry.path().string(), "_B09_60m.jp2") && b[ESA_S2_Image_Operator::DT_B09]) {
					splitJP2(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B09, data_resolution);
				}
			}
		}
		// Files within an L1C product with a 60 m resolution.
		for (const auto &r60m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/")) {
			if (endswith(r60m_entry.path().string(), "_B01.jp2") && b[ESA_S2_Image_Operator::DT_B01]) {
				splitJP2(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B01, data_resolution);
			} else if (endswith(r60m_entry.path().string(), "_B09.jp2") && b[ESA_S2_Image_Operator::DT_B09]) {
				splitJP2(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B09, data_resolution);
			} else if (endswith(r60m_entry.path().string(), "_B10.jp2") && b[ESA_S2_Image_Operator::DT_B10]) {
				splitJP2(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B10, data_resolution);
			}
		}
		// Sinergise's S2Cloudless classification map within an L1C product, with a 60 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/S2CLOUDLESS_DATA/R60m/")) {
			for (const auto &s2c_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/S2CLOUDLESS_DATA/R60m/")) {
				if (endswith(s2c_entry.path().string(), "_prediction.png") && b[ESA_S2_Image_Operator::DT_SS2C]) {
					splitPNG(s2c_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2C, data_resolution);
				} else if (endswith(s2c_entry.path().string(), "_probability.png") && b[ESA_S2_Image_Operator::DT_SS2CC]) {
					splitPNG(s2c_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2CC, data_resolution);
				}
			}
		}
		// Split NASA GSFC at 10 m resolution.
		if (std::filesystem::is_directory(granule_entry.path().string() + "/GSFC/")) {
			for (const auto &classification_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/GSFC/")) {
				data_resolution = ESA_S2_Image_Operator::DR_10M;
				if (endswith(classification_entry.path().string(), "label.tif") && b[ESA_S2_Image_Operator::DT_GSFC]) {
					splitTIF(classification_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_GSFC, data_resolution);
				}
			}
		}
	}

	// Split Baetens & Hagolle classification maps with a 60 m resolution.
	if (std::filesystem::is_directory(path_dir_in.string() + "/ref_dataset/Classification/")) {
		for (const auto &classification_entry: std::filesystem::directory_iterator(path_dir_in.string() + "/ref_dataset/Classification/")) {
			data_resolution = ESA_S2_Image_Operator::DR_60M;
			if (endswith(classification_entry.path().string(), "classification_map.tif") && b[ESA_S2_Image_Operator::DT_BHC]) {
				splitTIF(classification_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_BHC, data_resolution);
			}
		}
	}
	// Split Francis & Mrziglod & Sidiropoulos classification maps with a 20 m resolution.
	if (std::filesystem::is_directory(path_dir_in.string() + "/ref_dataset_mrziglod20/")) {
		for (const auto &classification_entry: std::filesystem::directory_iterator(path_dir_in.string() + "/ref_dataset_mrziglod20/")) {
			data_resolution = ESA_S2_Image_Operator::DR_20M;
			if (endswith(classification_entry.path().string(), "classification_map.png") && b[ESA_S2_Image_Operator::DT_FMSC]) {
				splitPNG(classification_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_FMSC, data_resolution);
			}
		}
	}
	// Split IPL-UV DL-L8S2-UV classification maps with a 10 m resolution.
	if (std::filesystem::is_regular_file(path_dir_in.string() + "/dluvclouds_rgbiswir.tif") && b[ESA_S2_Image_Operator::DT_DL_L8S2_UV]) {
		std::filesystem::path fpath(path_dir_in.string() + "/dluvclouds_rgbiswir.tif");
		data_resolution = ESA_S2_Image_Operator::DR_10M;
		splitTIF(fpath, path_dir_out, op, ESA_S2_Image_Operator::DT_DL_L8S2_UV, data_resolution);
	}

	return true;
}

/**
 * @brief Polygon-fill the subtile mask. Each pixel of the subtile mask corresponds to a sub-tile of the whole product.
 * @param[in] poly Reference to the polygon to fill onto the mask.
 * @param[in] pixel_size_div Effective pixel size, accounting the overlap.
 * @return 2D array of values (0 - skipped, 1 - sub-tile within area of interest).
 */
std::vector<std::vector<unsigned char>> fill_poly_overlap(Polygon<int> &poly, float pixel_size_div) {
	// Inspired by
	//   http://alienryderflex.com/polygon_fill/
	int i, j;
	float xf;
	std::vector<int> node_x;
	Vector<int> pixel;
	Vector<float> local_a, local_b;

	AABB<int> poly_aabb = poly.get_aabb();
	Vector<int> poly_dim = poly_aabb.vmax - poly_aabb.vmin;
	// Axis-aligned bounding box in the local reference frame.
	AABB<int> local_aabb(
		Vector<int>(0, 0),
		Vector<int>(ceil(((float) poly_dim.x) / pixel_size_div),
			ceil(((float) poly_dim.y) / pixel_size_div))
	);

	// Initialize the subtile mask.
	std::vector<std::vector<unsigned char>> subtile_mask(local_aabb.vmax.x, std::vector<unsigned char>(local_aabb.vmax.y, 0));

	for (pixel.y=local_aabb.vmin.y; pixel.y < local_aabb.vmax.y; pixel.y++) {
		node_x.clear();
		// Build a list of nodes.
		j = poly.size() - 1;
		for (i=0; i<(int)poly.size(); i++) {
			local_a.x = ((float) poly[i].x - poly_aabb.vmin.x) / pixel_size_div;
			local_a.y = ((float) poly[i].y - poly_aabb.vmin.y) / pixel_size_div;
			local_b.x = ((float) poly[j].x - poly_aabb.vmin.x) / pixel_size_div;
			local_b.y = ((float) poly[j].y - poly_aabb.vmin.y) / pixel_size_div;

			if ((local_a.y < (float) pixel.y && local_b.y >= (float) pixel.y)
				|| (local_b.y < (float) pixel.y && local_a.y >= (float) pixel.y)) {
				// Calculate the x-coordinate of the intersection between AB and y = pixel.y.
				xf = local_a.x + (pixel.y - local_a.y) / (local_b.y - local_a.y) * (local_b.x - local_a.x);
				node_x.push_back((int) xf);
			}
			j = i;
		}

		// Sort the nodes.
		std::sort(node_x.begin(), node_x.end());

		// Fill pixels between node pairs.
		for (i=0; i<(int)node_x.size(); i+=2) {
			if (node_x[i] >= local_aabb.vmax.x)
				break;
			if (node_x[i + 1] >= local_aabb.vmin.x) {
				if (node_x[i] < local_aabb.vmin.x)
					node_x[i] = local_aabb.vmin.x;
				if (node_x[i + 1] > local_aabb.vmax.x)
					node_x[i + 1] = local_aabb.vmax.x;
				for (pixel.x=node_x[i]; pixel.x<=node_x[i + 1]; pixel.x++) {
					// Fill the pixel covered by the polygon.
					subtile_mask[pixel.x][pixel.y] = 1;
					// Also fill the immediate surrounding pixels.
					if (pixel.x > 0)
						subtile_mask[pixel.x-1][pixel.y] = 1;
					if (pixel.x < local_aabb.vmax.x)
						subtile_mask[pixel.x+1][pixel.y] = 1;
					if (pixel.y > 0)
						subtile_mask[pixel.x][pixel.y-1] = 1;
					if (pixel.y < local_aabb.vmax.y)
						subtile_mask[pixel.x][pixel.y+1] = 1;
				}
			}
		}
	}
	return subtile_mask;
}

/**
 * @brief Fill the entire subtile mask. Each pixel of the subtile mask corresponds to a sub-tile of the whole product.
 * @param[in] image_aabb Reference to the axis-aligned bounding box of the whole product.
 * @param[in] tile_size_div Effective pixel size, accounting the overlap.
 * @return 2D array of values 1 - all sub-tiles within the area of interest.
 */
std::vector<std::vector<unsigned char>> fill_whole(const AABB<int> &image_aabb, float tile_size_div) {
	std::vector<Vector<int>> subtiles;
	Vector<int> p;

	Vector<int> img_dim = image_aabb.vmax - image_aabb.vmin;
	// Axis-aligned bounding box in the local reference frame.
	AABB<int> local_aabb(
		Vector<int>(0, 0),
		Vector<int>(ceil(((float) img_dim.x) / tile_size_div),
			ceil(((float) img_dim.y) / tile_size_div))
	);

	// Initialize the subtile mask.
	std::vector<std::vector<unsigned char>> subtile_mask(local_aabb.vmax.x, std::vector<unsigned char>(local_aabb.vmax.y, 1));

	return subtile_mask;
}

/**
 * @brief Extract the projection, geo-coordinates from the Sentinel-2 product. Produce the subtile mask for processing a subset of the product.
 * @param[in] path_in Reference to the axis-aligned bounding box of the whole product.
 * @param[in] image_aabb Reference to the axis-aligned bounding box of the whole product.
 * @param[in] tile_size_div Effective pixel size, accounting the overlap.
 */
void ESA_S2_Image::extract_geo(const std::filesystem::path &path_in, const AABB<int> &image_aabb, float tile_size_div) {
	GDALDataset *p_dataset = (GDALDataset *) GDALOpen(path_in.string().c_str(), GA_ReadOnly);
	if (p_dataset == NULL)
		throw RasterException(path_in, "Failed to load with GDAL");
	if (p_dataset->GetProjectionRef() != NULL) {
		std::cout << "Projection: " << p_dataset->GetProjectionRef() << std::endl;
	}

	// If there's an area of interest polygon, then fill the subtile mask with the polygon.
	subtile_mask.clear();
	if (wkt_geom_aoi.length() > 0) {
		std::cout << "Projecting AOI polygon into pixel coordinates." << std::endl;

		OGRGeometry *p_geom = nullptr;

		wkt_to_geom(wkt_geom_aoi, &p_geom);
		aoi_poly = proj_coords_to_raster<int>(p_geom, p_dataset);
		// Remove the last point, which is identical to the first.
		aoi_poly.remove(aoi_poly.size() - 1);
		// Increase the size of the polygon, to include overlap.
		aoi_poly.scale(1.0f + f_overlap);
		// Only keep the part of the polygon which is inside the raster.
		aoi_poly.clip_to_aabb(image_aabb);

		AABB<int> aabb = aoi_poly.get_aabb();
		aabb_buf = aabb.buffer(tile_size * f_overlap);
		aabb_buf.vmin.x /= image_aabb.vmax.x;
		aabb_buf.vmin.y /= image_aabb.vmax.y;
		aabb_buf.vmax.x /= image_aabb.vmax.x;
		aabb_buf.vmax.y /= image_aabb.vmax.y;

		//! \todo Use GDAL for reading raster data, too.

		if (aoi_poly.size() > 0) {
			subtile_mask = fill_poly_overlap(aoi_poly, tile_size_div);
		} else {
			GDALClose(p_dataset);
			throw RasterException(path_in, "No overlap between the area of interest polygon and raster");
		}
	// Otherwise take all the subtiles.
	} else {
		aabb_buf = AABB<float>(0, 0, 1, 1);
		subtile_mask = fill_whole(image_aabb, tile_size_div);
	}
	GDALClose(p_dataset);
}

//! \todo Generalize the splitting logic, to deduplicate code.

bool ESA_S2_Image::splitJP2(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution) {
	ESA_S2_Band_JP2_Image img_src;
	bool retval = true;

	float div_f = 1.0f;
	if (data_resolution == ESA_S2_Image_Operator::DR_20M)
		div_f = 2.0f;
	else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
		div_f = 6.0f;

	// With increased overlap, the effective subtile size is reduced.
	float tile_size_div = (tile_size - tile_size * f_overlap) / div_f;

	img_src.set_deflate_level(deflate_factor);
	img_src.set_num_threads(num_threads);

	// Propagate overlap factor for NetCDF metadata.
	img_src.f_overlap = f_overlap;
	// Assign product name from the input path.
	img_src.product_name = get_product_name_from_path(path_in);

	// Either load the full image or load only the header.
	if (read_tiled)
		retval &= img_src.load_header(path_in);
	else
		retval &= img_src.load_whole(path_in);

	std::cout << "Processing " << path_in << std::endl;

	Vector<int> p;

	// Extract image geo-coordinates, project area of interest polygon into pixel coordinates,
	// and produce a subtile mask, unless all of this has already been done.
	if (!geo_extracted) {
		std::cout << "Extracting geo-coordinates." << std::endl;
		AABB<int> image_aabb(img_src.main_geometry);
		extract_geo(path_in, image_aabb, tile_size_div);
		geo_extracted = true;
	}

	for (p.x=0; p.x<(int)subtile_mask.size(); p.x++) {
		for (p.y=0; p.y<(int)subtile_mask[p.x].size(); p.y++) {
			if (subtile_mask[p.x][p.y] == 1) {
				// Coordinates in the source image (possibly with different dimensions).
				int sx0 = aabb_buf.vmin.x * img_src.main_geometry.width() + floor(tile_size_div * p.x);
				int sy0 = aabb_buf.vmin.y * img_src.main_geometry.height() + floor(tile_size_div * p.y);
				int sx1 = ceil(sx0 + tile_size_div);
				int sy1 = ceil(sy0 + tile_size_div);

				// It's possible that due to rounding errors, the tile would no longer be square.
				// For this case, we'll crop the additional row / column of pixels to square the tile once again.
				if (sx1 - sx0 > sy1 - sy0)
					sx1 = sx0 + sy1 - sy0;
				else if (sy1 - sy0 > sx1 - sx0)
					sy1 = sy0 + sx1 - sx0;

				// Account for overlap.
				sx1 += tile_size * f_overlap / div_f;
				sy1 += tile_size * f_overlap / div_f;

				// Subset the source image.
				if (read_tiled)
					img_src.load_subset(path_in, sx0, sy0, sx1, sy1);
				else
					img_src.subset_whole(sx0, sy0, sx1, sy1);

				// Remap pixel values for SCL.
				if (data_type == ESA_S2_Image_Operator::DT_SCL) {
					if (scl_value_map != nullptr)
						img_src.remap_values(scl_value_map, max_scl_value);
					// Scale SCL with point filter.
					img_src.set_resampling_filter("point");
					img_src.scale_to((unsigned int) (tile_size / f_downscale));
				} else {
					// Scale other images with sinc filter.
					img_src.set_resampling_filter(resampling_method_name);
					img_src.scale_to((unsigned int) (tile_size / f_downscale));
				}

				if (img_src.subset->rows() != tile_size || img_src.subset->columns() != tile_size) {
					std::cout << "Invalid geometry " << img_src.subset->rows() << "x" << img_src.subset->columns() << " for subtile " << p.x << ", " << p.y << std::endl;
				}

				std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;

				ss_path_out << path_dir_out.string() << "/tile_" << p.x << "_" << p.y << "/";

				std::filesystem::create_directories(ss_path_out.str());

				// Save PNG.
				if (store_png) {
					ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << "tile" << "_" << p.x << "_" << p.y << ".png";
					img_src.save(ss_path_out_png.str());
				}
				// Add to NetCDF.
				ss_path_out_nc << ss_path_out.str() << extract_index_date(path_in) << "_" << "tile" << "_" << p.x << "_" << p.y << ".nc";
				img_src.add_to_netcdf(ss_path_out_nc.str(), ESA_S2_Image_Operator::data_type_name[data_type]);

				// Potential post-processing of the file.
				if (!op(ss_path_out.str(), data_type))
					return false;
			}
		}
	}

	return retval;
}

bool ESA_S2_Image::splitTIF(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution) {
	ESA_S2_Image_Operator::data_type_t tmp_data_type = data_type;
	TIF_Image img_src;
	bool retval = true;

	float div_f = 1.0f;
	if (data_resolution == ESA_S2_Image_Operator::DR_20M)
		div_f = 2.0f;
	else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
		div_f = 6.0f;

	float tile_size_div = (tile_size - tile_size * f_overlap) / div_f;

	img_src.set_deflate_level(deflate_factor);
	img_src.set_num_threads(num_threads);

	// Propagate overlap factor for NetCDF metadata.
	img_src.f_overlap = f_overlap;

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	std::cout << "Processing " << path_in << std::endl;

	// NOTE:: Assume square images and square tiles.
	int sx0, sy0, sx1, sy1;

	// Iterate over tiles in the output raster.
	Vector<int> p;
	for (p.x=0; p.x<(int)subtile_mask.size(); p.x++) {
		for (p.y=0; p.y<(int)subtile_mask[p.x].size(); p.y++) {
			if (subtile_mask[p.x][p.y] == 1) {
				// Coordinates in the source image (possibly with different dimensions).
				sx0 = aabb_buf.vmin.x * img_src.main_geometry.width() + floor(tile_size_div * p.x);
				sy0 = aabb_buf.vmin.y * img_src.main_geometry.height() + floor(tile_size_div * p.y);
				sx1 = ceil(sx0 + tile_size_div);
				sy1 = ceil(sy0 + tile_size_div);

				// It's possible that due to rounding errors, the tile would no longer be square.
				// For this case, we'll crop the additional row / column of pixels to square the tile once again.
				if (sx1 - sx0 > sy1 - sy0)
					sx1 = sx0 + sy1 - sy0;
				else if (sy1 - sy0 > sx1 - sx0)
					sy1 = sy0 + sx1 - sx0;

				// Account for overlap.
				sx1 += tile_size * f_overlap / div_f;
				sy1 += tile_size * f_overlap / div_f;

				// Load the source image.
				img_src.load_subset(path_in, sx0, sy0, sx1, sy1);

				// Remap pixel values from BHC, FMC or MAJAC into SCL and then from SCL into the desired classes.
				// This helps to ensure that there's only a single place in code which is responsible for the mapping
				// and that the mapping is configurable.
				if (data_type == ESA_S2_Image_Operator::DT_BHC) {
					img_src.remap_values(ESA_S2_Image_Operator::bhc_scl_value_map, sizeof(ESA_S2_Image_Operator::bhc_scl_value_map));
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				} else if (data_type == ESA_S2_Image_Operator::DT_FMC) {
					img_src.remap_values(ESA_S2_Image_Operator::fmc_scl_value_map, sizeof(ESA_S2_Image_Operator::fmc_scl_value_map));
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				} else if (data_type == ESA_S2_Image_Operator::DT_MAJAC) {
					CNES_MAJA_CLM_TIF::remap_majac_values(&img_src);
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				} else if (data_type == ESA_S2_Image_Operator::DT_GSFC) {
					img_src.remap_values(ESA_S2_Image_Operator::gsfc_scl_value_map, sizeof(ESA_S2_Image_Operator::gsfc_scl_value_map));
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				} else if (data_type == ESA_S2_Image_Operator::DT_DL_L8S2_UV) {
					img_src.remap_values(ESA_S2_Image_Operator::dl_l8s2_uv_scl_value_map, sizeof(ESA_S2_Image_Operator::dl_l8s2_uv_scl_value_map));
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				}
				if (tmp_data_type == ESA_S2_Image_Operator::DT_SCL) {
					if (scl_value_map != nullptr)
						img_src.remap_values(scl_value_map, max_scl_value);
					// Scale with point filter.
					img_src.set_resampling_filter("point");
					img_src.scale_to((unsigned int) (tile_size / f_downscale));
				} else {
					// Scale other images with sinc filter.
					img_src.set_resampling_filter(resampling_method_name);
					img_src.scale_to((unsigned int) (tile_size / f_downscale));
				}

				if (img_src.subset->rows() != tile_size || img_src.subset->columns() != tile_size) {
					std::cout << "Invalid geometry " << img_src.subset->rows() << "x" << img_src.subset->columns() << " for subtile " << p.x << ", " << p.y << std::endl;
				}

				std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;

				ss_path_out << path_dir_out.string() << "/tile_" << p.x << "_" << p.y << "/";

				std::filesystem::create_directories(ss_path_out.str());

				// Save PNG.
				if (store_png) {
					ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << "tile" << "_" << p.x << "_" << p.y << ".png";
					img_src.save(ss_path_out_png.str());
				}
				// Add to NetCDF.
				ss_path_out_nc << ss_path_out.str() << extract_index_date(path_in) << "_" << "tile" << "_" << p.x << "_" << p.y << ".nc";
				img_src.add_to_netcdf(ss_path_out_nc.str(), ESA_S2_Image_Operator::data_type_name[data_type]);

				// Potential post-processing of the file.
				if (!op(ss_path_out.str(), data_type))
					return false;
			}
		}
	}

	return retval;
}

bool ESA_S2_Image::splitPNG(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution) {
	ESA_S2_Image_Operator::data_type_t tmp_data_type = data_type;
	PNG_Image img_src;
	bool retval = true;

	float div_f = 1.0f;
	if (data_resolution == ESA_S2_Image_Operator::DR_20M)
		div_f = 2.0f;
	else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
		div_f = 6.0f;

	float tile_size_div = (tile_size - tile_size * f_overlap) / div_f;

	// Propagate overlap factor for NetCDF metadata.
	img_src.f_overlap = f_overlap;

	img_src.set_deflate_level(deflate_factor);
	img_src.set_num_threads(num_threads);

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	std::cout << "Processing " << path_in << std::endl;

	// NOTE:: Assume square images and square tiles.
	int sx0, sy0, sx1, sy1;

	// Iterate over tiles in the output raster.
	Vector<int> p;
	for (p.x=0; p.x<(int)subtile_mask.size(); p.x++) {
		for (p.y=0; p.y<(int)subtile_mask[p.x].size(); p.y++) {
			if (subtile_mask[p.x][p.y] == 1) {
				// Coordinates in the source image (possibly with different dimensions).
				sx0 = aabb_buf.vmin.x * img_src.main_geometry.width() + floor(tile_size_div * p.x);
				sy0 = aabb_buf.vmin.y * img_src.main_geometry.height() + floor(tile_size_div * p.y);
				sx1 = ceil(sx0 + tile_size_div);
				sy1 = ceil(sy0 + tile_size_div);

				// It's possible that due to rounding errors, the tile would no longer be square.
				// For this case, we'll crop the additional row / column of pixels to square the tile once again.
				if (sx1 - sx0 > sy1 - sy0)
					sx1 = sx0 + sy1 - sy0;
				else if (sy1 - sy0 > sx1 - sx0)
					sy1 = sy0 + sx1 - sx0;

				// Account for overlap.
				sx1 += tile_size * f_overlap / div_f;
				sy1 += tile_size * f_overlap / div_f;

				// Load the source image.
				img_src.load_subset(path_in, sx0, sy0, sx1, sy1);

				// Remap pixel values from SS2C or FMSC into SCL and then from SCL into the desired classes.
				// This helps to ensure that there's only a single place in code which is responsible for the mapping
				// and that the mapping is configurable.
				if (data_type == ESA_S2_Image_Operator::DT_SS2C) {
					img_src.remap_values(ESA_S2_Image_Operator::ss2c_scl_value_map, sizeof(ESA_S2_Image_Operator::ss2c_scl_value_map));
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				} else if (data_type == ESA_S2_Image_Operator::DT_FMSC) {
					img_src.remap_values(ESA_S2_Image_Operator::fmsc_scl_value_map, sizeof(ESA_S2_Image_Operator::fmsc_scl_value_map));
					tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
				}
				if (tmp_data_type == ESA_S2_Image_Operator::DT_SCL) {
					if (scl_value_map != nullptr)
						img_src.remap_values(scl_value_map, max_scl_value);
					// Scale with point filter.
					img_src.set_resampling_filter("point");
					img_src.scale_to((unsigned int) (tile_size / f_downscale));
				} else {
					// Scale other images with sinc filter.
					img_src.set_resampling_filter(resampling_method_name);
					img_src.scale_to((unsigned int) (tile_size / f_downscale));
				}

				if (img_src.subset->rows() != tile_size || img_src.subset->columns() != tile_size) {
					std::cout << "Invalid geometry " << img_src.subset->rows() << "x" << img_src.subset->columns() << " for subtile " << p.x << ", " << p.y << std::endl;
				}

				std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;

				ss_path_out << path_dir_out.string() << "/tile_" << p.x << "_" << p.y << "/";

				std::filesystem::create_directories(ss_path_out.str());


				// Save PNG.
				if (store_png) {
					ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << "tile" << "_" << p.x << "_" << p.y << ".png";
					img_src.save(ss_path_out_png.str());
				}
				// Add to NetCDF.
				ss_path_out_nc << ss_path_out.str() << extract_index_date(path_in) << "_" << "tile" << "_" << p.x << "_" << p.y << ".nc";
				img_src.add_to_netcdf(ss_path_out_nc.str(), ESA_S2_Image_Operator::data_type_name[data_type]);

				// Potential post-processing of the file.
				if (!op(ss_path_out.str(), data_type))
					return false;
			}
		}
	}

	return retval;
}
