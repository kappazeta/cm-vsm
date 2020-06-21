#include "raster/esa_s2.hpp"
#include "raster/esa_s2_scl_jp2.hpp"
#include "raster/esa_s2_band_jp2.hpp"

#include "util/text.hpp"


ESA_S2_Image::ESA_S2_Image(): tile_size(256), scl_value_map(nullptr) {}
ESA_S2_Image::~ESA_S2_Image() {}

void ESA_S2_Image::set_tile_size(int tile_size) {
	tile_size = tile_size;
}

void ESA_S2_Image::set_scl_class_map(unsigned char *class_map) {
	scl_value_map = class_map;
}

bool ESA_S2_Image::process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op) {
	ESA_S2_Image_Operator::data_resolution_t data_resolution;

	for (const auto &granule_entry: std::filesystem::directory_iterator(path_dir_in.string() + "/GRANULE/")) {
		data_resolution = ESA_S2_Image_Operator::DR_10M;
		for (const auto &r10m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/R10m/")) {
			//! \todo Map these
			if (endswith(r10m_entry.path().string(), "_TCI_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_TCI, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_AOT_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_AOT, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_WVP_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_WVP, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B02_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B02, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B03_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B03, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B04_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B04, data_resolution);
			} else if (endswith(r10m_entry.path().string(), "_B08_10m.jp2")) {
				split(r10m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B08, data_resolution);
			}
		}

		data_resolution = ESA_S2_Image_Operator::DR_20M;
		for (const auto &r20m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/R20m/")) {
			if (endswith(r20m_entry.path().string(), "_SCL_20m.jp2")) {
				split(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_SCL, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B05_20m.jp2")) {
				split(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B05, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B06_20m.jp2")) {
				split(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B06, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B8A_20m.jp2")) {
				split(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B8A, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B11_20m.jp2")) {
				split(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B11, data_resolution);
			} else if (endswith(r20m_entry.path().string(), "_B12_20m.jp2")) {
				split(r20m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B12, data_resolution);
			}
		}

		data_resolution = ESA_S2_Image_Operator::DR_60M;
		for (const auto &r60m_entry: std::filesystem::directory_iterator(granule_entry.path().string() + "/IMG_DATA/R60m/")) {
			if (endswith(r60m_entry.path().string(), "_B01_60m.jp2")) {
				split(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B01, data_resolution);
			} else if (endswith(r60m_entry.path().string(), "_B09_60m.jp2")) {
				split(r60m_entry.path(), path_dir_out, op, ESA_S2_Image_Operator::DT_B09, data_resolution);
			}
		}
	}
}

bool ESA_S2_Image::split(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution) {
	ESA_S2_Band_JP2_Image img_src;
	int div_f = 1;
	bool retval = true;

	if (data_resolution == ESA_S2_Image_Operator::DR_20M)
		div_f = 2;
	else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
		div_f = 6;

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	int w = img_src.main_geometry.width();
	int h = img_src.main_geometry.height();
	int tile_size_20m = 2 * tile_size / div_f;

	std::cout << "Processing " << path_in << std::endl;

	// Subset the image, and store the subsets in a dedicated directory.
	for (int y=0; y<h; y+=tile_size_20m) {
		std::cout << " " << y << std::endl;
		for (int x=0; x<w; x+=tile_size_20m) {
			// std::cout << "Tile " << x << ", " << y << std::endl;
			img_src.load_subset(path_in, x, y, x + tile_size_20m, y + tile_size_20m);

			// Remap pixel values for SCL.
			if (data_type == ESA_S2_Image_Operator::DT_SCL) {
				if (scl_value_map != nullptr)
					img_src.remap_values(scl_value_map);
				// Scale SCL with point filter.
				img_src.scale(div_f, true);
			} else {
				// Scale other images with sinc filter.
				img_src.scale(div_f, false);
			}

			std::ostringstream ss_path_out;

			if (data_resolution == ESA_S2_Image_Operator::DR_20M)
				ss_path_out << path_dir_out.string() << "/tile_" << x << "_" << y << "/";
			else if (data_resolution == ESA_S2_Image_Operator::DR_10M)
				ss_path_out << path_dir_out.string() << "/tile_" << x / 2 << "_" << y / 2 << "/";
			else if (data_resolution == ESA_S2_Image_Operator::DR_60M)
				ss_path_out << path_dir_out.string() << "/tile_" << x * 3 + x / tile_size_20m << "_" << y * 3 + y / tile_size_20m << "/";

			std::filesystem::create_directories(ss_path_out.str());

			ss_path_out << path_in.stem().string() << "_" << x << "_" << y << ".png";

			img_src.save(ss_path_out.str());
			// Potential post-processing of the file.
			if (!op(ss_path_out.str(), data_type))
				return false;
		}
	}

	std::cout << path_in << std::endl;

	return true;
}
