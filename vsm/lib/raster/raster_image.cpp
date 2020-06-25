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

#include "raster/raster_image.hpp"
#include <netcdf.h>
#include <climits>


RasterImage::RasterImage(): main_depth(0), main_num_components(0), subset(nullptr) {}
RasterImage::~RasterImage() {
	clear();
}

void RasterImage::clear() {
	if (subset != nullptr) {
		delete subset;
		subset = nullptr;
	}
}

std::ostream& operator<<(std::ostream &out, const RasterImage& img) {
	if (img.subset != nullptr) {
		Magick::Geometry geom = img.subset->size();
		return out << "RasterImage(x0=" << geom.xOff()
			<< ", y0=" << geom.yOff()
			<< ", w=" << geom.width()
			<< ", h=" << geom.height()
			<< ", c=" << img.main_num_components
			<< ", d=" << img.subset->depth() << ")";
	}

	return out << "RasterImage()";
}

bool RasterImage::save(const std::filesystem::path &path) {
	if (subset != nullptr) {
		subset->write(path.string());
		return true;
	}
	return false;
}

bool RasterImage::scale(float f, bool point_filter) {
	if (subset != nullptr) {
		// No scaling needed?
		if (f >= 0.999f && f <= 1.001f)
			return true;

		Magick::Geometry geom_orig = subset->size();
		Magick::Geometry geom_new(geom_orig.width() * f, geom_orig.height() * f);
		if (point_filter)
			subset->filterType(Magick::PointFilter);
		else
			subset->filterType(Magick::SincFilter);
		subset->resize(geom_new);
		return true;
	}
	return false;
}

void RasterImage::remap_values(const unsigned char *values) {
	//! \todo Implement support for remapping colors.
	if (subset != nullptr) {
		unsigned int w = subset->columns();
		unsigned int h = subset->rows();
		unsigned int size = w * h;

		Magick::PixelPacket *px = subset->getPixels(0, 0, w, h);

		register double src_val, dst_val;

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(px[i]).shade();
			dst_val = values[(unsigned char) (255 * src_val)] / 255.0;
			px[i] = Magick::ColorGray(dst_val);
		}

		subset->syncPixels();
	}
}

bool RasterImage::add_to_netcdf(const std::filesystem::path &path, const std::string &name_in_netcdf, int deflate_level) {
	int ncid = 0, varid = 0;
	int dimids[2] = {0, 0};
	int retval;

	if (subset == nullptr) {
		std::cerr << "Nothing to add to NetCDF file \"" << path << "\", for the subset is empty" << std::endl;
		return false;
	}

	unsigned int w = subset->columns();
	unsigned int h = subset->rows();
	unsigned int size = w * h;

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

	try {
		// Open or create the file.
		if (std::filesystem::exists(path)) {
			if ((retval = nc_open(path.string().c_str(), NC_WRITE, &ncid)))
				throw NCException("failed to open", path, retval);
		} else {
			if ((retval = nc_create(path.string().c_str(), NC_NOCLOBBER | NC_NETCDF4, &ncid)))
				throw NCException("failed to create", path, retval);
		}

		// Define dimensions.
		retval = nc_def_dim(ncid, "x", w, &dimids[0]);
		if (retval != NC_NOERR && retval != NC_ENAMEINUSE) {
			std::ostringstream ss;
			ss << "failed to create dimension x=" << w;
			throw NCException(ss.str(), path, retval);
		}
		retval = nc_def_dim(ncid, "y", h, &dimids[1]);
		if (retval != NC_NOERR && retval != NC_ENAMEINUSE) {
			std::ostringstream ss;
			ss << "failed to create dimension y=" << h;
			throw NCException(ss.str(), path, retval);
		}

		//! \todo Support for groups.

		// Variable dimensions and data type.
		int nd = sizeof(dimids) / sizeof(dimids[0]);
		int dt = NC_FLOAT;

		// Define the variable.
		if ((retval = nc_def_var(ncid, name_in_netcdf.c_str(), dt, nd, dimids, &varid))) {
			std::ostringstream ss;
			ss << "failed to create dimension " << nd << "D variable \"" << name_in_netcdf << "\"";
			throw NCException(ss.str(), path, retval);
		}
		if ((retval = nc_def_var_deflate(ncid, varid, NC_SHUFFLE, 1, deflate_level))) {
			std::ostringstream ss;
			ss << "failed to set deflation level " << deflate_level << " for variable \"" << name_in_netcdf << "\"";
			throw NCException(ss.str(), path, retval);
		}
		if ((retval = nc_enddef(ncid)))
			throw NCException("failed to finish a definition", path, retval);

		Magick::PixelPacket *src_px = subset->getPixels(0, 0, w, h);
		register double src_val;

		// Store content.
		float dst_px[size];

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(src_px[i]).shade();
			dst_px[i] = (float) src_val;
		}

		if ((retval = nc_put_var_float(ncid, varid, dst_px))) {
			std::ostringstream ss;
			ss << "failed to store an array of " << w << " x " << h << " float values in a variable";
			throw NCException(ss.str(), path, retval);
		}

	} catch (NCException &e) {
		if (e.nc_retval != NC_ENAMEINUSE)
			std::cerr << e.what() << std::endl;

		// Close the file.
		if ((retval = nc_close(ncid)))
			std::cerr << "Failed to close NetCDF file \"" << path << "\", error " << retval << std::endl;
		return false;
	}

	// Close the file.
	if ((retval = nc_close(ncid))) {
		std::cerr << "Failed to close NetCDF file \"" << path << "\", error " << retval << std::endl;
		return false;
	}
	return true;
}

