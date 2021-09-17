//! @file
//! @brief Processing of ESA S2 L2A products
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


/**
 * @brief An operator class for raster or vector layers, which are related to ESA Sentinel-2 images.
 */
class ESA_S2_Image_Operator {
public:
	/**
	 * @brief Raster data type.
	 */
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
		DT_GSFC,    ///< NASA GSFC, vector layer
		DT_DL_L8S2_UV,	///< IPL-UV DL-L8S2-UV, binary classification map, 10 m
		DT_COUNT	///< Total number of data type options.
	};

	/**
	 * @brief Ground resolution of the data.
	 */
	enum data_resolution_t {
		DR_UNKNOWN,	///< The resolution is not known.
		DR_10M,	///< Each pixel is 10 x 10 \f$ m^2 \f$.
		DR_20M,	///< Each pixel is 20 x 20 \f$ m^2 \f$.
		DR_60M, ///< Each pixel is 60 x 60 \f$ m^2 \f$.
		DR_VECTOR, ///< It's a vector layer (no inherent ground resolution).
		DR_COUNT	///< Total number of data resolution options.
	};

	static const std::string data_type_name[DT_COUNT];	///< List of supported band names.
	static const unsigned char bhc_scl_value_map[9]; ///< Baetens & Hagolle to Sen2Cor classification map.
	static const unsigned char fmc_scl_value_map[6];	///< FMask to Sen2Cor classification map.
	static const unsigned char ss2c_scl_value_map[3];	///< Sinergise S2Cloudless to Sen2Cor classification map.
	static const unsigned char fmsc_scl_value_map[4];	///< Francis & Mrziglod & Sidiropoulos to Sen2Cor classification map.
	static const unsigned char gsfc_scl_value_map[6];	///< NASA GSFC to Sen2Cor classification map.
	static const unsigned char dl_l8s2_uv_scl_value_map[4];	///< IPL-UV DL-L8S2-UV, binary classification map.

	/**
	 * Callback for potential post-processing on the sub-tiles.
	 * @param path Path to the sub-tile subdirectory.
	 * @param type File type.
	 * @return True on success, false to abort sub-tile processing.
	 */
	virtual bool operator()(const std::filesystem::path &path, data_type_t type) { (void) path; (void) type; return false; }
};


/**
 * @brief Empty image operator, serves as a placeholder.
 */
class EmptyImageOperator: public ESA_S2_Image_Operator {
public:
	/**
	 * Image operator which simply returns successfully without any actual processing.
	 */
	bool operator()(const std::filesystem::path &path, data_type_t type) { (void) path; (void) type; return true; }
};


/**
 * @brief Class for an ESA Sentinel-2 image.
 */
class ESA_S2_Image {
	public:
		ESA_S2_Image();
		~ESA_S2_Image();

		/**
		 * Set sub-tile size. Each sub-tile is \f$ tile_size \times tile_size \f$ pixels.
		 * @param tile_size Length of tile edge in pixels.
		 */
		void set_tile_size(int tile_size);

		/**
		 * Set down-scaling factor for subsampling the image.
		 * Useful for sub-splitting an image with 10 m resolution to match other images with 60 m resolution (\f$ f = 6 \f$).
		 */
		void set_downscale_factor(int f);

		/**
		 * Set deflate factor.
		 * @note For optimal performance during training or prediction, compression should be disabled (deflate factor set to `0`).
		 * @param d Deflate factor, between 0 and 9. Specify 0 for no compression, or 9 for maximum compression.
		 */
		void set_deflate_factor(int d);

		/**
		 * Set overlap factor.
		 * If the model architecture produces artifacts at sub-tile edges, overlapping sub-tiles can be used for prediction, and then the results can be merged into a single output image later.
		 * @note The optimal overlap factor is defined by the filter sizes in the model architecture. For a classical UNet architecture for a \f$ 512 \times 512 px^2 \f$ sub-tile, the optimal overlap would be a border of 32 pixels.
		 * @param f Specify `0.0f` for no overlap, `0.0625f` for 32 pixels of a \f$ 512 \times 512 px^2 \f$ sub-tile, or `0.5f` for the maximum overlap of half a sub-tile.
		 */
		void set_overlap_factor(float f);

		/**
		 * Set resampling method for all bands except classification masks to one of the following: `sinc`, `cubic`, `box`, `point`.
		 * For classification masks, the nearest neighbour resampling method (either `box` or `point`) is used.
		 * @note The resampling methods used for training and prediction must be identical.
		 * @param m Reference to the name of the resampling method.
		 */
		void set_resampling_method(const std::string &m);

		/**
		 * Set class map for remapping from Sen2Cor classifications.
		 * @param class_map Pointer to unsigned char array of 13 class indices, with the last index for unmatched classes.
		 */
		void set_scl_class_map(unsigned char *class_map);

		/**
		 * Enable / disable the storage of individual bands in PNG files.
		 * Useful for debugging or visualization.
		 * @param enabled True to store PNG files, False to skip them.
		 */
		void set_png_output(bool enabled);

		/**
		 * Enable / disable tiled reading (reduced RAM footprint, at the cost of longer processing time).
		 * @param enabled False to read the whole JP2 file into RAM, True to read the JP2 file in tiles.
		 */
		void set_tiled_input(bool enabled);

		/**
		 * Set the number of threads to parallelize to.
		 * @param num_threads Number of threads (0 for default, negative to use all available threads).
		 */
		void set_num_threads(int num_threads);

		/**
		 * Extract S2 product name from file path.
		 * @param[in] path Reference to the path to a Sentinel-2 product.
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
		int num_threads;	///< Number of threads to parallelize to.

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

