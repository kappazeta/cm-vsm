//! @file
//! @brief Generic raster image
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

#include <iostream>
#include <filesystem>
#include <Magick++.h>
#include <netcdf.h>
#include <vector>


/**
 * @brief Class for exceptions related to raster files.
 */
class RasterException: public std::exception {
	public:
		/**
		 * @param[in] msg Reference to the error message.
		 * @param[in] path Reference to the path of the raster file which this error relates to.
		 */
		RasterException(const std::string &msg, const std::filesystem::path &path) {
			message = msg;
			r_path = path;

			std::ostringstream ss;
			ss << "Raster file " << r_path << ": " << message;
			full_message.assign(ss.str());
		}

		std::string message;	///< Error message.
		std::filesystem::path r_path;	///< Path to the raster file which this error relates to.

		/**
		 * Pointer to the full message C string.
		 */
		virtual const char* what() const throw() {
			return full_message.c_str();
		}

	protected:
		std::string full_message;	///< Full human-readable message.
};

/**
 * @brief Class for exceptions related to NetCDF files.
 */
class NCException: public std::exception {
	public:
		/**
		 * @param[in] msg Reference to the error message.
		 * @param[in] path Reference to the path of the NetCDF file which this error relates to.
		 * @param[in] retval Error code from the NetCDF library.
		 */
		NCException(const std::string &msg, const std::filesystem::path &path, int retval) {
			message = msg;
			nc_path = path;
			nc_retval = retval;

			std::ostringstream ss;
			ss << "NetCDF file " << nc_path << ": " << message << ", " << nc_strerror(nc_retval);
			full_message.assign(ss.str());
		}

		std::string message;	///< Error message.
		std::filesystem::path nc_path;	///< Path to the NetCDF file which this error relates to.
		int nc_retval;	///< Error code from the NetCDF library.

		/**
		 * Pointer to the full message C string.
		 */
		virtual const char* what() const throw() {
			return full_message.c_str();
		}

	protected:
		std::string full_message;	///< Full human-readable message.
};

/**
 * @brief An 8-bit RGB pixel class.
 */
class PixelRGB8 {
	public:
		/**
		 * Initialize an RGB pixel.
		 * @param[in] _r Red component, 0 - 255.
		 * @param[in] _g Green component, 0 - 255.
		 * @param[in] _b Blue component, 0 - 255.
		 */
		PixelRGB8(unsigned char _r, unsigned char _g, unsigned char _b);

		/**
		 * Initialize an RGB pixel, based on input from GraphicsMagick++.
		 * @param[in] px Reference to a GraphicsMagick++ PixelPacket as an input for the pixel components.
		 */
		PixelRGB8(const Magick::PixelPacket &px);

		/**
		 * Initialize a grayscale pixel.
		 * @param[in] c Grayscale component, 0 - 255.
		 */
		PixelRGB8(unsigned char c);

		/**
		 * Initialize a black pixel.
		 */
		PixelRGB8();

		/**
		 * Deinitialize the pixel.
		 */
		~PixelRGB8();

		unsigned char r;	///< Red component, 0 - 255.
		unsigned char g;	///< Green component, 0 - 255.
		unsigned char b;	///< Blue component, 0 - 255.

		/**
		 * Set pixel value from an std::vector of components.
		 * @param[in] components Reference to a vector of 3 components: R, G, B.
		 * @return Reference to the pixel class instance.
		 */
		PixelRGB8 &set(const std::vector<int> &components);

		/**
		 * Set pixel value from a GraphicsMagick::PixelPacket.
		 * @param[in] px Reference to the PixelPacket to use as an input.
		 * @return Reference to the pixel class instance.
		 */
		PixelRGB8 &set(const Magick::PixelPacket &px);

		/**
		 * Assign pixel components from another pixel instance.
		 * @param[in] a Reference to the source.
		 * @return Reference to the pixel class instance.
		 */
		PixelRGB8 &operator=(const PixelRGB8 &a);

		/**
		 * Check if two pixel instances have equal components.
		 * @param[in] Reference to a pixel instance to compare against.
		 * @return True if all components are equal, otherwise false.
		 */
		bool operator==(const PixelRGB8 &a);
};

/**
 * @brief 1D raster buffer class for a pan-chromatic image.
 * @tparam T Data type for pixel values.
 */
template <typename T>
class RasterBufferPan {
	public:
		/**
		 * Initialize the buffer.
		 * @param size Number of pixels to allocate for the buffer.
		 */
		RasterBufferPan(unsigned long int size);

		/**
		 * Free the buffer.
		 */
		~RasterBufferPan();

		T *v;	///< Pointer to an array of pixel values.
};

/**
 * @brief 1D raster buffer class for an RGB colour image.
 * @tparam T Data type for pixel values.
 */
template <typename T>
class RasterBufferRGB {
	public:
		/**
		 * Initialize the buffer.
		 * @param size Number of pixels to allocate for the buffer.
		 */
		RasterBufferRGB(unsigned long int size);

		/**
		 * Free the buffer.
		 */
		~RasterBufferRGB();

		T *r;	///< Pointer to an array of pixel red components.
		T *g;	///< Pointer to an array of pixel green components.
		T *b;	///< Pointer to an array of pixel blue components.
};

/**
 * @brief A generic raster class.
 */
class RasterImage {
	public:
		/**
		 * Initialize an empty raster.
		 */
		RasterImage();

		/**
		 * De-initialize the raster.
		 */
		~RasterImage();

		/**
		 * Create a grayscale image.
		 * @param geometry Image geometry
		 * @param pixel_depth Bits per pixel
		 * @param background_value Pixel value to fill the image with
		 * @return Pointer to the new image.
		 */
		Magick::Image *create_grayscale(const Magick::Geometry &geometry, int pixel_depth, int background_value);

		/**
		 * Abstract function for loading image from file in subclasses.
		 */
		virtual bool load(const std::filesystem::path &) {
			return false;
		}

		/**
		 * Save image to file.
		 * @param path Path to the file to save to.
		 * @return True on success, false on failure.
		 */
		bool save(const std::filesystem::path &path);

		/**
		 * Add image to a NetCDF file as a variable.
		 * @param path Path to the NetCDF file.
		 * @param name_in_netcdf Variable name in NetCDF.
		 * @return True on success, false on failure.
		 */
		bool add_to_netcdf(const std::filesystem::path &path, const std::string &name_in_netcdf);

		/**
		 * Clear the image and release memory.
		 */
		void clear();

		std::string product_name;	///< Product name, for NetCDF metadata.
		std::string resampling_filter_name;	///< Name of the resampling filter used, for NetCDF metadata.

		Magick::Image *subset;	///< Pointer to image content.
		Magick::Geometry main_geometry;	///< Image geometry
		unsigned char main_depth;	///< Pixel depth in bits.
		unsigned char main_num_components;	///< Number of channels (1 for grayscale, 3 for RGB) in the raster image.

		float f_overlap;	///< Overlap factor [0.0f, 0.5f], for NetCDF metadata.

		/**
		 * Set the deflate level to use for NetCDF storage.
		 * @param level Deflate level
		 * @return Configured deflate level, between 0 and 9.
		 */
		unsigned int set_deflate_level(unsigned int level);

		/**
		 * Set the filter used for resampling.
		 * @param filter_name Filter name, one of the following: "sinc", "cubic", "box", "point".
		 * @return Native enum value for the resampling filter.
		 */
		Magick::FilterTypes set_resampling_filter(const std::string &filter_name);

		/**
		 * Set the number of threads to parallelize to.
		 * @param num_threads Number of threads (0 for default, negative to use all available threads).
		 */
		void set_num_threads(int num_threads);

		/**
		 * Scale the image by a factor.
		 * @param f Factor to scale by
		 * @return True on success, false on failure.
		 */
		bool scale_f(float f);

		/**
		 * Scale the image to a specific size (assuming a square image)
		 * @param size Number of pixels to resize a side of the image to.
		 * @return True on success, false on failure.
		 */
		bool scale_to(unsigned int size);

		/**
		 * Multiply pixels by a factor.
		 * @param f Factor to multiply with.
		 * @return True on success, false on failure.
		 */
		bool multiply(float f);

		/**
		 * Remap pixel values (assuming a classification mask).
		 * @param values Pointer to an array of new pixel values. Current pixel values are taken as index. Last value is used for out of range indices.
		 * @param max_value Maximum pixel value supported by the values argument. Assumes that any pixel values exceeding this value is remapped to the last value in the values argument.
		 */
		void remap_values(const unsigned char *values, unsigned char max_value);

	protected:
		int num_threads;	///< Number of threads to parallelize to.

	private:
		Magick::FilterTypes resampling_filter;	///< Enum index of the resampling filter used.
		unsigned int deflate_level;	///< Deflate level [0, 9] for the NetCDF variable.
		float scaling_factor;	///< Scaling factor used for resampling the image for storage in NetCDF.

		/**
		 * Add a layer to an open NetCDF file.
		 * @param ncid ID of the open NetCDF instance.
		 * @param path Path to the NetCDF file (used for errors and exceptions).
		 * @param name_in_netcdf Name of the variable to store the image in.
		 * @param w Image width, in pixels.
		 * @param h Image height, in pixels.
		 * @param dimids Pointer to an array with the IDs of the X and Y dimensions.
		 * @param src_px Pointer to image content.
		 * @return NetCDF variable ID.
		 */
		int add_layer_to_netcdf(int ncid, const std::filesystem::path &path, const std::string &name_in_netcdf, unsigned int w, unsigned int h, const int *dimids, unsigned char nd, const void *src_px);
};

/**
 * Produce a human-readable string of a RasterImage class instance.
 * @param out Output stream
 * @param img Raster image class instance
 * @return Reference to the stream instance with the added content.
 */
std::ostream &operator<<(std::ostream &out, const RasterImage &img);

