// Processing of ESA S2 L2A products
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

#include "raster/esa_s2.hpp"
#include "raster/esa_s2_scl_jp2.hpp"
#include "raster/esa_s2_band_jp2.hpp"
#include "raster/bhc_tif.hpp"
#include "raster/png_image.hpp"

#include "util/text.hpp"


const std::string ESA_S2_Image_Operator::data_type_name[ESA_S2_Image_Operator::DT_COUNT] = {
	"TCI", "SCL", "AOT", "B01", "B02", "B03", "B04", "B05", "B06", "B07", "B08", "B8A", "B09", "B10", "B11", "B12",	"WVP",
	"GML", "S2CC", "S2CS", "FMC", "SS2C", "SS2CC", "BHC", "FMSC"
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

ESA_S2_Image::ESA_S2_Image(): tile_size(512), scl_value_map(nullptr), max_scl_value(12), f_downscale(1) {}
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
		if (std::filesystem::is_directory(granule_entry.path().string() + "/S2CLOUDLESS_DATA/")) {
			for (const auto &fmask_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/S2CLOUDLESS_DATA/")) {
				if (endswith(fmask_entry.path().string(), "_prediction.png") && b[ESA_S2_Image_Operator::DT_SS2C]) {
					splitPNG(fmask_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2C, data_resolution);
				} else if (endswith(fmask_entry.path().string(), "_probability.png") && b[ESA_S2_Image_Operator::DT_SS2CC]) {
					splitPNG(fmask_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SS2CC, data_resolution);
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

	return true;
}

bool ESA_S2_Image::splitJP2(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution) {
	ESA_S2_Band_JP2_Image img_src;
	bool retval = true;

	float div_f = 1.0f;
	if (data_resolution == ESA_S2_Image_Operator::DR_20M)
		div_f = 2.0f;
	else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
		div_f = 6.0f;

	float tile_size_div = tile_size / div_f;

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	int w = img_src.main_geometry.width();
	int h = img_src.main_geometry.height();

	std::cout << "Processing " << path_in << std::endl;

	// Subset the image, and store the subsets in a dedicated directory.
	int xi, yi;
	float xf, yf;
	int xi0 = 0, yi0 = 0;
	for (yi=yi0, yf=yi*tile_size_div; yf<(float)h; yf+=tile_size_div,yi++) {
		for (xi=xi0, xf=xi*tile_size_div; xf<(float)w; xf+=tile_size_div,xi++) {
			// Round-down the x0, y0 coordinates and round-up the x1, y1 coordinates..
			img_src.load_subset(path_in, (int) xf, (int) yf, (int) (xf + tile_size_div + 0.5f), (int) (yf + tile_size_div + 0.5f));

			// Remap pixel values for SCL.
			if (data_type == ESA_S2_Image_Operator::DT_SCL) {
				if (scl_value_map != nullptr)
					img_src.remap_values(scl_value_map, max_scl_value);
				// Scale SCL with point filter.
				img_src.scale_to((unsigned int) (tile_size / f_downscale), true);
			} else {
				// Scale other images with sinc filter.
				img_src.scale_to((unsigned int) (tile_size / f_downscale), false);
			}

			std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;

			ss_path_out << path_dir_out.string() << "/tile_" << xi << "_" << yi << "/";

			std::filesystem::create_directories(ss_path_out.str());

			ss_path_out_nc << ss_path_out.str() << "bands.nc";
			ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << (int) xf << "_" << (int) yf << ".png";

			// Save PNG.
			img_src.save(ss_path_out_png.str());
			// Add to NetCDF.
			img_src.add_to_netcdf(ss_path_out_nc.str(), ESA_S2_Image_Operator::data_type_name[data_type]);

			// Potential post-processing of the file.
			if (!op(ss_path_out_png.str(), data_type))
				return false;
		}
	}

	std::cout << path_in << std::endl;

	return retval;
}

bool ESA_S2_Image::splitTIF(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution) {
	ESA_S2_Image_Operator::data_type_t tmp_data_type = data_type;
	BHC_TIF_Image img_src;
	bool retval = true;

	float div_f = 1.0f;
	if (data_resolution == ESA_S2_Image_Operator::DR_20M)
		div_f = 2.0f;
	else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
		div_f = 6.0f;

	float tile_size_div = tile_size / div_f;

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	int w = img_src.main_geometry.width();
	int h = img_src.main_geometry.height();

	std::cout << "Processing " << path_in << std::endl;

	// Subset the image, and store the subsets in a dedicated directory.
	int xi, yi;
	float xf, yf;
	int xi0 = 0, yi0 = 0;
	for (yi=yi0, yf=yi*tile_size_div; yf<(float)h; yf+=tile_size_div,yi++) {
		for (xi=xi0, xf=xi*tile_size_div; xf<(float)w; xf+=tile_size_div,xi++) {
			// Round-down the x0, y0 coordinates and round-up the x1, y1 coordinates..
			img_src.load_subset(path_in, (int) xf, (int) yf, (int) (xf + tile_size_div + 0.5f), (int) (yf + tile_size_div + 0.5f));

			// Remap pixel values from BHC or FMC into SCL and then from SCL into the desired classes.
			// This helps to ensure that there's only a single place in code which is responsible for the mapping
			// and that the mapping is configurable.
			if (data_type == ESA_S2_Image_Operator::DT_BHC) {
				img_src.remap_values(ESA_S2_Image_Operator::bhc_scl_value_map, sizeof(ESA_S2_Image_Operator::bhc_scl_value_map));
				tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
			} else if (data_type == ESA_S2_Image_Operator::DT_FMC) {
				img_src.remap_values(ESA_S2_Image_Operator::fmc_scl_value_map, sizeof(ESA_S2_Image_Operator::fmc_scl_value_map));
				tmp_data_type = ESA_S2_Image_Operator::DT_SCL;
			}
			if (tmp_data_type == ESA_S2_Image_Operator::DT_SCL) {
				if (scl_value_map != nullptr)
					img_src.remap_values(scl_value_map, max_scl_value);
				// Scale with point filter.
				img_src.scale_to((unsigned int) (tile_size / f_downscale), true);
			} else {
				// Scale other images with sinc filter.
				img_src.scale_to((unsigned int) (tile_size / f_downscale), false);
			}

			std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;

			ss_path_out << path_dir_out.string() << "/tile_" << xi << "_" << yi << "/";

			std::filesystem::create_directories(ss_path_out.str());

			ss_path_out_nc << ss_path_out.str() << "bands.nc";
			ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << (int) xf << "_" << (int) yf << ".png";

			// Save PNG.
			img_src.save(ss_path_out_png.str());
			// Add to NetCDF.
			img_src.add_to_netcdf(ss_path_out_nc.str(), ESA_S2_Image_Operator::data_type_name[data_type]);

			// Potential post-processing of the file.
			if (!op(ss_path_out_png.str(), data_type))
				return false;
		}
	}

	std::cout << path_in << std::endl;

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

	float tile_size_div = tile_size / div_f;

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	int w = img_src.main_geometry.width();
	int h = img_src.main_geometry.height();

	std::cout << "Processing " << path_in << std::endl;

	// Subset the image, and store the subsets in a dedicated directory.
	int xi, yi;
	float xf, yf;
	int xi0 = 0, yi0 = 0;
	for (yi=yi0, yf=yi*tile_size_div; yf<(float)h; yf+=tile_size_div,yi++) {
		for (xi=xi0, xf=xi*tile_size_div; xf<(float)w; xf+=tile_size_div,xi++) {
			// Round-down the x0, y0 coordinates and round-up the x1, y1 coordinates..
			img_src.load_subset(path_in, (int) xf, (int) yf, (int) (xf + tile_size_div + 0.5f), (int) (yf + tile_size_div + 0.5f));

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
				img_src.scale_to((unsigned int) (tile_size / f_downscale), true);
			} else {
				// Scale other images with sinc filter.
				img_src.scale_to((unsigned int) (tile_size / f_downscale), false);
			}

			std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;

			ss_path_out << path_dir_out.string() << "/tile_" << xi << "_" << yi << "/";

			std::filesystem::create_directories(ss_path_out.str());

			ss_path_out_nc << ss_path_out.str() << "bands.nc";
			ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << (int) xf << "_" << (int) yf << ".png";

			// Save PNG.
			img_src.save(ss_path_out_png.str());
			// Add to NetCDF.
			img_src.add_to_netcdf(ss_path_out_nc.str(), ESA_S2_Image_Operator::data_type_name[data_type]);

			// Potential post-processing of the file.
			if (!op(ss_path_out_png.str(), data_type))
				return false;
		}
	}

	std::cout << path_in << std::endl;

	return retval;
}
