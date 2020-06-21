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


class ESA_S2_Image_Operator {
public:
	typedef enum data_type_t {
		DT_TCI,	///< True Color Image (8 bit RGB), 10 m
		DT_SCL,	///< Scene Classification Image (8 bit), 20 m
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
		DT_GML	///< Vector mask layer
	};

	typedef enum data_resolution_t {
		DR_UNKNOWN,
		DR_10M,
		DR_20M,
		DR_60M,
		DR_VECTOR
	};

	virtual bool operator()(const std::filesystem::path &path, data_type_t type) { return false; }
};


class ESA_S2_Image {
	public:
		ESA_S2_Image();
		~ESA_S2_Image();

		int tile_size;

		void set_tile_size(int tile_size);

		void set_scl_class_map(unsigned char *class_map);

		bool process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op);

		bool split(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);

	protected:
		unsigned char *scl_value_map;
};

