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
		DT_MAJAC,	///< MAJA classification map (8 bit), 10 m
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

	static const std::string data_type_name[DT_COUNT];	///< List of supported band names.
	static const unsigned char bhc_scl_value_map[9]; ///< Baetens & Hagolle to Sen2Cor classification map.
	static const unsigned char fmc_scl_value_map[6];	///< FMask to Sen2Cor classification map.
	static const unsigned char ss2c_scl_value_map[3];	///< Sinergise S2Cloudless to Sen2Cor classification map.
	static const unsigned char fmsc_scl_value_map[4];	///< Francis & Mrziglod & Sidiropoulos to Sen2Cor classification map.

	/**
	 * Callback for potential post-processing on the sub-tiles.
	 * @param path Path to the sub-tile subdirectory.
	 * @param type File type.
	 * @return True on success, false to abort sub-tile processing.
	 */
	virtual bool operator()(const std::filesystem::path &path, data_type_t type) { (void) path; (void) type; return false; }
};


class EmptyImageOperator: public ESA_S2_Image_Operator {
public:
	/**
	 * Image operator which simply returns successfully without any actual processing.
	 */
	bool operator()(const std::filesystem::path &path, data_type_t type) { (void) path; (void) type; return true; }
};


class ESA_S2_Image {
	public:
		ESA_S2_Image();
		~ESA_S2_Image();

		/**
		 * Set sub-tile size in pixels.
		 */
		void set_tile_size(int tile_size);

		/**
		 * Set down-scaling factor for subsampling the image.
		 * Useful for sub-splitting an image with 10 m resolution to match other images with 60 m resolution (f = 6).
		 */
		void set_downscale_factor(int f);

		/**
		 * Set deflate factor [0, 9].
		 */
		void set_deflate_factor(int d);

		/**
		 * Set overlap factor [0.0f, 0.5f].
		 */
		void set_overlap_factor(float f);

		/**
		 * Set resampling method for all bands except classification masks to one of the following: "sinc", "cubic", "box", "point".
		 */
		void set_resampling_method(const std::string &m);

		/**
		 * Set class map for remapping from Sen2Cor classifications.
		 * @param class_map Pointer to unsigned char array of 13 class indices, with the last index for unmatched classes.
		 */
		void set_scl_class_map(unsigned char *class_map);

		/**
		 * Enable / disable the storage of PNG files.
		 * @param enabled True to store PNG files, False to skip them.
		 */
		void set_png_output(bool enabled);

		/**
		 * Enable / disable tiled reading (reduced RAM footprint, at the cost of longer processing time).
		 * @param enabled False to read the whole JP2 file into RAM, True to read the JP2 file in tiles.
		 */
		void set_tiled_input(bool enabled);

		/**
		 * Extract S2 product name from file path.
		 * @return Product name as a string.
		 */
		static std::string get_product_name_from_path(const std::filesystem::path &path);

		/**
		 * Process a Sentinel-2 L1C or L2A image.
		 * @param path_dir_in Path to the .SAFE directory.
		 * @param path_dir_out Path to the output directory.
		 * @param op Operator for class remapping and any other post-processing.
		 * @param bands List of band names (ESA_S2_Image_Operator::data_type_name) to process.
		 * @return True on success, false on failure.
		 */
		bool process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, std::vector<std::string> bands);

	protected:
		unsigned int tile_size;	///< Sub-tile size, in pixels.

		unsigned char *scl_value_map;	///< Pointer to class map from Sen2Cor into a custom classification scheme.
		unsigned char max_scl_value;	///< Maximum allowed index for the class map (12 for Sen2Cor).

		int f_downscale;	///< Factor for down-scaling (subsampling) the image.
		int deflate_factor;	///< Deflate factor for NetCDF storage.

		float f_overlap;	///< Overlap between sub-tiles.
		
		std::string resampling_method_name;	///< Name of the resampling method used for all bands except classification masks.

		bool store_png;	///< Whether to store intermediate output in PNG files or not.
		bool read_tiled;	///< Whether to read JP2 files in tiles, or to read full images into RAM.

		/**
		 * Split a JP2 file into sub-tiles.
		 * @param path_in Path to the JP2 file.
		 * @param path_dir_out Path to the output directory to store the sub-tiles.
		 * @param op Operator for class remapping and any other post-processing.
		 * @param data_type Band type.
		 * @param data_resolution Band resolution.
		 * @return True on success, false on failure.
		 */
		bool splitJP2(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);

		/**
		 * Split a TIFF file into sub-tiles.
		 * @param path_in Path to the TIFF file.
		 * @param path_dir_out Path to the output directory to store the sub-tiles.
		 * @param op Operator for class remapping and any other post-processing.
		 * @param data_type Band type.
		 * @param data_resolution Band resolution.
		 * @return True on success, false on failure.
		 */
		bool splitTIF(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);

		/**
		 * Split a PNG file into sub-tiles.
		 * @param path_in Path to the TIFF file.
		 * @param path_dir_out Path to the output directory to store the sub-tiles.
		 * @param op Operator for class remapping and any other post-processing.
		 * @param data_type Band type.
		 * @param data_resolution Band resolution.
		 * @return True on success, false on failure.
		 */
		bool splitPNG(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, ESA_S2_Image_Operator &op, ESA_S2_Image_Operator::data_type_t data_type, ESA_S2_Image_Operator::data_resolution_t data_resolution);
};

