//! @file
//! @brief Processing of KappaZeta S2 raster products
//
// Copyright 2022 KappaZeta Ltd.
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

#include "util/geometry.hpp"


/**
 * @brief An operator class for raster or vector layers, which are related to KappaZeta S1 / S2 raster products.
 */
class KZ_S2_TIF_Image_Operator {
public:
	/**
	 * @brief Raster data type.
	 */
	enum data_type_t {
		DT_KZ_S2_B02 = 0,	///< 457.5 - 522.5 nm (32 bit), 10 m
		DT_KZ_S2_B03 = 1,	///< 542.5 - 577.5 nm (32 bit), 10 m
		DT_KZ_S2_B04 = 2,	///< 650 - 680 nm (32 bit), 10 m
		DT_KZ_S2_B08 = 3,	///< 784.5 - 899.5 nm (32 bit), 10 m
		DT_KZ_S2_B05 = 4,	///< 697.5 - 712.5 nm (32 bit), 20 m
		DT_KZ_S2_B06 = 5,	///< 732.5 - 747.5 nm (32 bit), 20 m
		DT_KZ_S2_B07 = 6,	///< 773 - 793 nm (32 bit), 20 m
		DT_KZ_S2_B8A = 7,	///< 855 - 875 nm (32 bit), 20 m
		DT_KZ_S2_B11 = 8,	///< 1565 - 1655 nm (32 bit), 20 m
		DT_KZ_S2_B12 = 9,	///< 2100 - 2280 nm (32 bit), 20 m
		DT_KZ_S2_NDVI = 10,	///< NDVI (32 bit), 10 m
		DT_KZ_S2_NDWI = 11,	///< NDWI (32 bit), 10 m
		DT_KZ_S2_NDVIRE = 12,	///< NDVIre (32 bit), 10 m
		DT_KZ_S2_TCW = 13,	///< TC Wetness (32 bit), 10 m
		DT_KZ_S2_TCV = 14,	///< TC Vegetation (32 bit), 10 m
		DT_KZ_S2_TCB = 15,	///< TC Brightness (32 bit), 10 m
		DT_KZ_S2_YEL = 16,	///< MISRA Yellow Vegetation (32 bit), 10 m
		DT_KZ_S2_PSRI = 17,	///< PSRI (32 bit), 10 m
		DT_KZ_S2_WRI = 18,	///< WRI (32 bit), 10 m
		DT_KZ_LABEL = 19,	///< Mask with labels, 10 m
		DT_KZ_S2_COUNT = 20	///< Total number of data type options.
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

	static const std::string data_type_name[DT_KZ_S2_COUNT];	///< List of supported band names.
	static const float scale_max[DT_KZ_S2_COUNT];	///< List of maximum values per band.

	/**
	 * Callback for potential post-processing on the sub-tiles.
	 * @param path Path to the sub-tile subdirectory.
	 * @return True on success, false to abort sub-tile processing.
	 */
	virtual bool operator()(const std::filesystem::path &path) { (void) path; return false; }
};


/**
 * @brief Empty image operator, serves as a placeholder.
 */
class KZEmptyImageOperator: public KZ_S2_TIF_Image_Operator {
public:
	/**
	 * Image operator which simply returns successfully without any actual processing.
	 */
	bool operator()(const std::filesystem::path &path) { (void) path; return true; }
};


/**
 * @brief Class for an ESA Sentinel-2 image.
 */
class KZ_S2_TIF_Image {
	public:
		KZ_S2_TIF_Image();
		~KZ_S2_TIF_Image();

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
		 * Enable / disable the storage of individual bands in PNG files.
		 * Useful for debugging or visualization.
		 * @param enabled True to store PNG files, False to skip them.
		 */
		void set_png_output(bool enabled);

		/**
		 * Set a WKT geometry of the area of interest which limits the sub-tiles.
		 * @param wkt_geom Reference to the WKT string which outlines the area of interest.
		 */
		void set_aoi_geometry(const std::string &wkt_geom);

		/**
		 * Extract S2 product name from file path.
		 * @param[in] path Reference to the path to a Sentinel-2 product.
		 * @return Product name as a string.
		 */
		static std::string get_product_name_from_path(const std::filesystem::path &path);

		/**
		 * Process a KZ S1 or S2 image.
		 * @param path_in Path to the .tif file.
		 * @param path_dir_out Path to the output directory.
		 * @param op Operator for class remapping and any other post-processing.
		 * @param bands List of band names (KZ_S2_TIF_Image_Operator::data_type_name) to process.
		 * @return True on success, false on failure.
		 */
		bool process(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, KZ_S2_TIF_Image_Operator &op, std::vector<std::string> bands);

	protected:
		unsigned int tile_size;	///< Sub-tile size, in pixels.

		int f_downscale;	///< Factor for down-scaling (subsampling) the image.
		int deflate_factor;	///< Deflate factor for NetCDF storage.

		float f_overlap;	///< Overlap between sub-tiles.

		std::string resampling_method_name;	///< Name of the resampling method used for all bands except classification masks.

		bool store_png;	///< Whether to store intermediate output in PNG files or not.
		bool read_tiled;	///< Whether to read JP2 files in tiles, or to read full images into RAM.
		int num_threads;	///< Number of threads to parallelize to.

		std::string wkt_geom_aoi;	///< Area of interest as WKT geometry.

		bool geo_extracted;	///< Whether the geo-coordinates have been extracted from at least one of the overlapping rasters.
		std::string proj_ref;	///< Projection reference, as extracted from the JP2 file.
		std::vector<std::vector<unsigned char>> subtile_mask;	///< Mask of subtiles to fill.
		AABB<float> aabb_buf;	///< Buffered axis-aligned bounding box surrounding the area of interest polygon, in relative image coordinates.
		Polygon<int> aoi_poly;	///< Area of interest polygon in pixel coordinates.

		void extract_geo(const std::filesystem::path &path_in, const AABB<int> &image_aabb, float tile_size_div);

		/**
		 * Split a TIFF file.
		 * @param path_in Path to the .tif file.
		 * @param path_dir_out Path to the output directory.
		 * @param op Operator for class remapping and any other post-processing.
		 * @param bands List of band IDs (KZ_S2_TIF_Image_Operator::data_type_t) to process.
		 * @return True on success, false on failure.
		 */
		bool split_tiff(const std::filesystem::path &path_dir_in, const std::filesystem::path &path_dir_out, KZ_S2_TIF_Image_Operator &op, std::vector<unsigned int> band_ids);
};

