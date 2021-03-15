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

#pragma once

#include <filesystem>
#include <vector>


class ESA_S2_Image_Operator {
public:
	enum data_type_t {
		DT_TCI,	///< True Color Image (8 bit RGB), 10 m
		DT_SCL,	///< Sen2Cor Scene Classification Image (8 bit), 20 m
		DT_AOT,	///< Aerosol Optical Thickness map (16 bit), 10 m
		DT_B01,	///< 433 - 453 nm (16 bit), 60 m
		DT_B02,	///< 457.5 - 522.5 nm (16 bit), 10 m
		DT_B03,	///< 542.5 - 577.5 nm (16 bit), 10 m
		DT_B04,	///< 650 - 680 nm (16 bit), 10 m
		DT_B05,	///< 697.5 - 712.5 nm (16 bit), 20 m
		DT_B06,	///< 732.5 - 747.5 nm (16 bit), 20 m
		DT_B07,	///< 773 - 793 nm (16 bit), 20 m
		DT_B08,	///< 784.5 - 899.5 nm (16 bit), 10 m
		DT_B8A,	///< 855 - 875 nm (16 bit), 20 m
		DT_B09,	///< 935 - 955 nm (16 bit), 60 m
		DT_B10,
		DT_B11,	///< 1565 - 1655 nm (16 bit), 20 m
		DT_B12,	///< 2100 - 2280 nm (16 bit), 20 m
		DT_WVP,	///< Water Vapour map (16 bit), 10 m
		DT_GML,	///< Vector mask layer
		DT_S2CC,	///< Sen2cor cloud probabilities (8 bit), 20 m
		DT_S2CS,	///< Sen2cor snow probabilities (8 bit), 20 m
		DT_FMC,	///< Fmask classification map, 20 m
		DT_SS2C,	///< Sinergise's S2Cloudless classification map (8 bit), 60 m
		DT_SS2CC,	///< Sinergise's S2Cloudless cloud probabilities (8 bit), 60 m
		DT_BHC,	///< Baetens & Hagolle classification map, 60 m
		DT_FMSC,	///< Francis & Mrziglod & Sidiropoulos classification map, 20 m
		DT_COUNT
	};

	enum data_resolution_t {
		DR_UNKNOWN,
		DR_10M,
		DR_20M,
		DR_60M,
		DR_VECTOR,
		DR_COUNT
	};

	static const std::string data_type_name[DT_COUNT];
	static const unsigned char bhc_scl_value_map[9];
	static const unsigned char fmc_scl_value_map[6];
	static const unsigned char ss2c_scl_value_map[3];
	static const unsigned char fmsc_scl_value_map[4];

	virtual bool operator()(const std::filesystem::path &path, data_type_t type) { (void) path; (void) type; return false; }
};


class ESA_S2_Image {
	public:
		ESA_S2_Image();
		~ESA_S2_Image();

		unsigned int tile_size;

		void set_tile_size(int tile_size);

		void set_downscale_factor(int f);

		void set_deflate_factor(int d);

		void set_overlap_factor(float f);

		void set_scl_class_map(unsigned char *class_map);

		static std::string get_product_name_from_path(const std::filesystem::path &path);

		bool process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, std::vector<std::string> bands);

		bool splitJP2(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);
		bool splitTIF(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);
		bool splitPNG(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);

	protected:
		unsigned char *scl_value_map;
		unsigned char max_scl_value;

		int f_downscale;
		int deflate_factor;
		float f_overlap;
};

