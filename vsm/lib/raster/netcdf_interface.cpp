// NetCDF raster image
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

#include "raster/netcdf_interface.hpp"
#include "util/datetime.hpp"
#include "version.hpp"
#include <climits>
#include <cstring>
#include <netcdf.h>


NetCDFInterface::NetCDFInterface():
	deflate_level(9)
{
}

NetCDFInterface::~NetCDFInterface() {
}

unsigned int NetCDFInterface::set_deflate_level(unsigned int level) {
	if (level >= 9)
		deflate_level = 9;
	else
		deflate_level = level;
	return deflate_level;
}

bool NetCDFInterface::has_layer(const std::filesystem::path &path, const std::string &name_in_netcdf) {
	int retval;
	int ncid = 0, varid = 0;
	bool layer_exists = false;

	if (nc_open(path.string().c_str(), NC_WRITE, &ncid)) {
		if (nc_inq_varid(ncid, name_in_netcdf.c_str(), &varid) == NC_NOERR)
			layer_exists = true;

		nc_close(ncid);
	}

	return layer_exists;
}

int NetCDFInterface::add_layer_to_file(int ncid, const std::filesystem::path &path, const std::string &name_in_netcdf, unsigned int w, unsigned int h, const int *dimids, unsigned char nd, const void *src_px, const RasterImage &image) {
	int varid = 0;
	int retval;

	int dt = NC_FLOAT;

	if (image.main_depth <= 8)
		dt = NC_UBYTE;

	// Define the variable.
	if (nc_inq_varid(ncid, name_in_netcdf.c_str(), &varid) != NC_NOERR) {
		retval = nc_def_var(ncid, name_in_netcdf.c_str(), dt, nd, dimids, &varid);
		if (retval != NC_NOERR && retval != NC_ENAMEINUSE) {
			std::ostringstream ss;
			ss << "failed to create dimension " << nd << "D variable \"" << name_in_netcdf << "\"";
			throw NCException(ss.str(), path, retval);
		}
		if (retval == NC_NOERR) {
			if ((retval = nc_def_var_deflate(ncid, varid, NC_SHUFFLE, 1, deflate_level))) {
				std::ostringstream ss;
				ss << "failed to set deflation level " << deflate_level << " for variable \"" << name_in_netcdf << "\"";
				throw NCException(ss.str(), path, retval);
			}
			if ((retval = nc_enddef(ncid)))
				throw NCException("failed to finish a definition", path, retval);
		}
	}

	// Store content.
	if (image.main_depth > 8) {
		if ((retval = nc_put_var_float(ncid, varid, (const float *) src_px))) {
			std::ostringstream ss;
			ss << "failed to store an array of " << w << " x " << h << " float values in a variable";
			throw NCException(ss.str(), path, retval);
		}
	} else {
		if ((retval = nc_put_var_ubyte(ncid, varid, (const unsigned char *) src_px))) {
			std::ostringstream ss;
			ss << "failed to store an array of " << w << " x " << h << " unsigned byte values in a variable";
			throw NCException(ss.str(), path, retval);
		}
	}

	// Variable attribute for scaling_factor.
	float scaling_factor = image.scaling_factor;
	if ((retval = nc_put_att(ncid, varid, "scaling_factor", NC_FLOAT, 1, &scaling_factor)))
		throw NCException("failed to put attribute scaling_factor to " + name_in_netcdf, path, retval);

	// Variable attribute for resampling method.
	if ((retval = nc_put_att_text(ncid, varid, "resampling_filter", image.resampling_filter_name.size(), image.resampling_filter_name.c_str())))
		throw NCException("failed to put attribute resampling_filter to " + name_in_netcdf, path, retval);

	// Variable attribute for last modified date-time.
	std::string last_modified_str = datetime_now_str();
	if ((retval = nc_put_att_text(ncid, varid, "last_modified", last_modified_str.size(), last_modified_str.c_str())))
		throw NCException("failed to put attribute last_modified to " + name_in_netcdf, path, retval);

	return varid;
}

bool NetCDFInterface::add_to_file(const std::filesystem::path &path, const std::string &name_in_netcdf, const RasterImage &image) {
	int ncid = 0;
	int dimids[2] = {0, 0};
	int retval;

	if (image.subset == nullptr) {
		std::cerr << "Nothing to add to NetCDF file \"" << path << "\", for the subset is empty" << std::endl;
		return false;
	}

	unsigned int w = image.subset->columns();
	unsigned int h = image.subset->rows();
	unsigned int c;
	unsigned int size = w * h;

	try {
		Magick::ImageType imgtype = image.subset->type();

		if (imgtype == Magick::GrayscaleType)
			c = 1;
		else if (imgtype == Magick::TrueColorType)
			c = 3;

		// Open or create the file.
		if (std::filesystem::exists(path)) {
			if ((retval = nc_open(path.string().c_str(), NC_WRITE, &ncid)))
				throw NCException("failed to open", path, retval);
		} else {
			if ((retval = nc_create(path.string().c_str(), NC_NOCLOBBER | NC_NETCDF4, &ncid)))
				throw NCException("failed to create", path, retval);
			// Global attribute for version number.
			if ((retval = nc_put_att_text(ncid, NC_GLOBAL, "version", strlen(CM_CONVERTER_VERSION_STR), CM_CONVERTER_VERSION_STR)))
				throw NCException("failed to put global attribute version", path, retval);
			// Global attribute for product name.
			if ((retval = nc_put_att_text(ncid, NC_GLOBAL, "product_name", image.product_name.size(), image.product_name.c_str())))
				throw NCException("failed to put global attribute product_name", path, retval);
			// Global attribute for overlap.
			float f_overlap = image.f_overlap;
			if ((retval = nc_put_att(ncid, NC_GLOBAL, "overlap", NC_FLOAT, 1, &f_overlap)))
				throw NCException("failed to put global attribute overlap", path, retval);
		}

		// Define dimensions.
		if (nc_inq_dimid(ncid, "x", &dimids[0]) != NC_NOERR) {
			retval = nc_def_dim(ncid, "x", w, &dimids[0]);
			if (retval != NC_NOERR && retval != NC_ENAMEINUSE) {
				std::ostringstream ss;
				ss << "failed to create dimension x=" << w;
				throw NCException(ss.str(), path, retval);
			}
		}
		if (nc_inq_dimid(ncid, "y", &dimids[1]) != NC_NOERR) {
			retval = nc_def_dim(ncid, "y", h, &dimids[1]);
			if (retval != NC_NOERR && retval != NC_ENAMEINUSE) {
				std::ostringstream ss;
				ss << "failed to create dimension y=" << h;
				throw NCException(ss.str(), path, retval);
			}
		}

		//! \todo Support for groups.

		// Variable dimensions and data type.
		int nd = sizeof(dimids) / sizeof(dimids[0]);

		Magick::PixelPacket *src_px = image.subset->getPixels(0, 0, w, h);
		unsigned int yw;

		// Store content.
		if (c == 1) {
			if (image.main_depth > 8) {
				RasterBufferPan<float> dst_px(size);

				for (unsigned int y=0; y<h; y++) {
					yw = y * w;
					for (unsigned int x=0; x<w; x++) {
						dst_px.v[yw + x] = ((float) src_px[yw + x].green) / MaxRGB;
					}
				}
				add_layer_to_file(ncid, path, name_in_netcdf, w, h, dimids, nd, (const void *) dst_px.v, image);
			} else {
				RasterBufferPan<unsigned char> dst_px(size);

				for (unsigned int y=0; y<h; y++) {
					yw = y * w;
					for (unsigned int x=0; x<w; x++) {
						dst_px.v[yw + x] = (int) (src_px[yw + x].green * 255 / MaxRGB);
					}
				}
				add_layer_to_file(ncid, path, name_in_netcdf, w, h, dimids, nd, (const void *) dst_px.v, image);
			}

		} else if (c == 3) {
			RasterBufferRGB<unsigned char> dst_px(size);

			for (unsigned int y=0; y<h; y++) {
				yw = y * w;
				for (unsigned int x=0; x<w; x++) {
					//! \todo TODO:: Figure out why pixels of an 8-bit image are stored as 16-bit values.
					// Is it due to the TrueColorType?
					dst_px.r[yw + x] = (int) (src_px[yw + x].red * 255 / 65535.0f);
					dst_px.g[yw + x] = (int) (src_px[yw + x].green * 255 / 65535.0f);
					dst_px.b[yw + x] = (int) (src_px[yw + x].blue * 255 / 65535.0f);
				}
			}

			add_layer_to_file(ncid, path, name_in_netcdf + "_R", w, h, dimids, nd, (const void *) dst_px.r, image);
			add_layer_to_file(ncid, path, name_in_netcdf + "_G", w, h, dimids, nd, (const void *) dst_px.g, image);
			add_layer_to_file(ncid, path, name_in_netcdf + "_B", w, h, dimids, nd, (const void *) dst_px.b, image);
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

