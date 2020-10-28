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
#include <climits>


template<typename T>
RasterBufferPan<T>::RasterBufferPan(unsigned long int size) {
	v = new T[size];
}
template<typename T>
RasterBufferPan<T>::~RasterBufferPan() {
	delete [] v;
}

template<typename T>
RasterBufferRGB<T>::RasterBufferRGB(unsigned long int size) {
	r = new T[size];
	g = new T[size];
	b = new T[size];
}
template<typename T>
RasterBufferRGB<T>::~RasterBufferRGB() {
	delete [] r;
	delete [] g;
	delete [] b;
}

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

Magick::Image *RasterImage::create_grayscale(const Magick::Geometry &geometry, int pixel_depth, int background_value) {
	clear();

	main_geometry = geometry;
	main_depth = pixel_depth;
	main_num_components = 1;

	subset = new Magick::Image(main_geometry, Magick::ColorGray(((double) background_value) / 255.0));
	subset->quiet(false);
	subset->type(Magick::GrayscaleType);
	subset->depth(pixel_depth);
	subset->endian(Magick::LSBEndian);

	return subset;
}

bool RasterImage::save(const std::filesystem::path &path) {
	if (subset != nullptr) {
		subset->write(path.string());
		return true;
	}
	return false;
}

bool RasterImage::scale_f(float f, bool point_filter) {
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

bool RasterImage::scale_to(unsigned int size, bool point_filter) {
	if (subset != nullptr) {
		Magick::Geometry geom_orig = subset->size();

		if (geom_orig.width() == size && geom_orig.height() == size)
			return true;

		Magick::Geometry geom_new(size, size);
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

		float src_val, dst_val;

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(px[i]).shade();
			dst_val = values[(unsigned char) (255 * src_val)] / 255.0f;
			px[i] = Magick::ColorGray(dst_val);
		}

		subset->syncPixels();
	}
}

int RasterImage::add_layer_to_netcdf(int ncid, const std::filesystem::path &path, const std::string &name_in_netcdf, unsigned int w, unsigned int h, const int *dimids, unsigned char nd, const void *dst_px, int deflate_level) {
	int varid = 0;
	int retval;

	int dt = NC_FLOAT;

	if (main_depth <= 8)
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
	if (main_depth > 8) {
		if ((retval = nc_put_var_float(ncid, varid, (const float *) dst_px))) {
			std::ostringstream ss;
			ss << "failed to store an array of " << w << " x " << h << " float values in a variable";
			throw NCException(ss.str(), path, retval);
		}
	} else {
		if ((retval = nc_put_var_ubyte(ncid, varid, (const unsigned char *) dst_px))) {
			std::ostringstream ss;
			ss << "failed to store an array of " << w << " x " << h << " unsigned byte values in a variable";
			throw NCException(ss.str(), path, retval);
		}
	}
	return varid;
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
	unsigned int c;
	unsigned int size = w * h;

	try {
		Magick::ImageType imgtype = subset->type();

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

		Magick::PixelPacket *src_px = subset->getPixels(0, 0, w, h);
		float src_val;

		// Store content.
		if (c == 1) {
			if (main_depth > 8) {
				unsigned int yw, fyw;

				RasterBufferPan<float> dst_px(size);

				for (unsigned int y=0; y<h; y++) {
					yw = y * w;
					fyw = (h - 1 - y) * w;
					for (unsigned int x=0; x<w; x++) {
						src_val = Magick::ColorGray(src_px[yw + x]).shade();
						dst_px.v[fyw + x] = src_val;
					}
				}

				add_layer_to_netcdf(ncid, path, name_in_netcdf, w, h, dimids, nd, (const void *) dst_px.v, deflate_level);
			} else {
				unsigned int yw, fyw;

				RasterBufferPan<unsigned char> dst_px(size);

				for (unsigned int y=0; y<h; y++) {
					yw = y * w;
					fyw = (h - 1 - y) * w;
					for (unsigned int x=0; x<w; x++) {
						src_val = Magick::ColorGray(src_px[yw + x]).shade();
						dst_px.v[fyw + x] = (int) (src_val * 255);
					}
				}

				add_layer_to_netcdf(ncid, path, name_in_netcdf, w, h, dimids, nd, (const void *) dst_px.v, deflate_level);
			}
		} else if (c == 3) {
			unsigned int yw, fyw;

			RasterBufferRGB<unsigned char> dst_px(size);

			for (unsigned int y=0; y<h; y++) {
				yw = y * w;
				fyw = (h - 1 - y) * w;
				for (unsigned int x=0; x<w; x++) {
					//! \todo TODO:: Figure out why pixels of an 8-bit image are stored as 16-bit values.
					// Is it due to the TrueColorType?
					dst_px.r[fyw + x] = (int) (src_px[yw + x].red * 255 / 65535.0f);
					dst_px.g[fyw + x] = (int) (src_px[yw + x].green * 255 / 65535.0f);
					dst_px.b[fyw + x] = (int) (src_px[yw + x].blue * 255 / 65535.0f);
				}
			}

			add_layer_to_netcdf(ncid, path, name_in_netcdf + "_R", w, h, dimids, nd, (const void *) dst_px.r, deflate_level);
			add_layer_to_netcdf(ncid, path, name_in_netcdf + "_G", w, h, dimids, nd, (const void *) dst_px.g, deflate_level);
			add_layer_to_netcdf(ncid, path, name_in_netcdf + "_B", w, h, dimids, nd, (const void *) dst_px.b, deflate_level);
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

