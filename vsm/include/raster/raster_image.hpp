// Generic raster image
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

#include <iostream>
#include <filesystem>
#include <Magick++.h>
#include <netcdf.h>


class NCException: public std::exception {
	public:
		NCException(const std::string &msg, const std::filesystem::path &path, int retval) {
			message = msg;
			nc_path = path;
			nc_retval = retval;

			std::ostringstream ss;
			ss << "NetCDF file " << nc_path << ": " << message << ", " << nc_strerror(nc_retval);
			full_message.assign(ss.str());
		}

		std::string message;
		std::filesystem::path nc_path;
		int nc_retval;

		virtual const char* what() const throw() {
			return full_message.c_str();
		}

	protected:
		std::string full_message;
};

template <typename T>
class RasterBufferPan {
	public:
		RasterBufferPan(unsigned long int size);
		~RasterBufferPan();

		T *v;
};

template <typename T>
class RasterBufferRGB {
	public:
		RasterBufferRGB(unsigned long int size);
		~RasterBufferRGB();

		T *r;
		T *g;
		T *b;
};

class RasterImage {
	public:
		RasterImage();
		~RasterImage();

		Magick::Image *create_grayscale(const Magick::Geometry &geometry, int pixel_depth, int background_value);

		virtual bool load(const std::filesystem::path &path) {}

		bool save(const std::filesystem::path &path);
		bool add_to_netcdf(const std::filesystem::path &path, const std::string &name_in_netcdf, int deflate_level);
		void clear();

		Magick::Image *subset;

		Magick::Geometry main_geometry;
		unsigned char main_depth;
		unsigned char main_num_components;

		bool scale_f(float f, bool point_filter);
		bool scale_to(unsigned int size, bool point_filter);
		void remap_values(const unsigned char *values);

	private:
		int add_layer_to_netcdf(int ncid, const std::filesystem::path &path, const std::string &name_in_netcdf, unsigned int w, unsigned int h, const int *dimids, unsigned char nd, const void *src_px, int deflate_level);
};

std::ostream &operator<<(std::ostream &out, const RasterImage &img);

