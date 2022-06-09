//! @file
//! @brief NetCDF raster image
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

#include <iostream>
#include <filesystem>

#include "raster/raster_image.hpp"


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
 * @brief An interface to manipulate NetCDF files.
 */
class NetCDFInterface {
	public:
		NetCDFInterface();
		~NetCDFInterface();

		/**
		 * Add image to a NetCDF file as a variable.
		 * @param path Path to the NetCDF file.
		 * @param name_in_netcdf Variable name in NetCDF.
		 * @return True on success, false on failure.
		 */
		bool add_to_file(const std::filesystem::path &path, const std::string &name_in_netcdf, const RasterImage &image);

		/**
		 * Check if the NetCDF file has a layer with a specific name.
		 * @param path Path to the NetCDF file.
		 * @param name_in_netcdf Variable name in NetCDF.
		 * @return True if the layer is present, otherwise false.
		 */
		bool has_layer(const std::filesystem::path &path, const std::string &name_in_netcdf);

		/**
		 * Set the deflate level to use for NetCDF storage.
		 * @param level Deflate level
		 * @return Configured deflate level, between 0 and 9.
		 */
		unsigned int set_deflate_level(unsigned int level);

	private:
		unsigned int deflate_level;	///< Deflate level [0, 9] for the NetCDF variable.

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
		int add_layer_to_file(int ncid, const std::filesystem::path &path, const std::string &name_in_netcdf, unsigned int w, unsigned int h, const int *dimids, unsigned char nd, const void *src_px, const RasterImage &image);
};

