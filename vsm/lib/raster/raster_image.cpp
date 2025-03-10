// Generic raster image
//
// Copyright 2021 - 2025 KappaZeta Ltd.
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
#include "raster/netcdf_interface.hpp"
#include "util/datetime.hpp"
#include "version.hpp"
#include <climits>
#include <cstring>

PixelRGB8::PixelRGB8(unsigned char _r, unsigned char _g, unsigned char _b):
	r(_r), g(_g), b(_b) {}
PixelRGB8::PixelRGB8(unsigned char c): r(c), g(c), b(c) {}
PixelRGB8::PixelRGB8(const Magick::PixelPacket &px) {
	set(px);
}
PixelRGB8::PixelRGB8(): r(0), g(0), b(0) {}
PixelRGB8::~PixelRGB8() {}

PixelRGB8 &PixelRGB8::operator=(const PixelRGB8 &a) {
	r = a.r;
	g = a.g;
	b = a.b;

	return *this;
}

bool PixelRGB8::operator==(const PixelRGB8 &a) {
	return (a.r == r && a.g == g && a.b == b);
}

PixelRGB8 &PixelRGB8::set(const std::vector<int> &components) {
	if (components.size() >= 1)
		r = components[0];
	if (components.size() >= 2)
		g = components[1];
	if (components.size() >= 3)
		b = components[2];

	return *this;
}
PixelRGB8 &PixelRGB8::set(const Magick::PixelPacket &px) {
	Magick::ColorRGB c(px);
	r = (unsigned char) (255 * c.red());
	g = (unsigned char) (255 * c.green());
	b = (unsigned char) (255 * c.blue());

	return *this;
}

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

RasterImage::RasterImage():
	subset(nullptr), main_depth(0), main_num_components(0), f_overlap(0.0f), scaling_factor(1.0f), num_threads(0),
	deflate_level(9)
{
	set_resampling_filter("");
}
RasterImage::~RasterImage() {
	clear();
}

unsigned int RasterImage::set_deflate_level(unsigned int level) {
	if (level >= 9)
		deflate_level = 9;
	else
		deflate_level = level;
	return deflate_level;
}

Magick::FilterTypes RasterImage::set_resampling_filter(const std::string &filter_name) {
	//! \todo Support other filters
	resampling_filter_name = filter_name;
	if (filter_name == "point") {
		resampling_filter = Magick::PointFilter;
	} else if (filter_name == "box") {
		resampling_filter = Magick::BoxFilter;
	} else if (filter_name == "linear") {
		resampling_filter = Magick::TriangleFilter;
	} else if (filter_name == "cubic") {
		resampling_filter = Magick::CubicFilter;
	} else if (filter_name == "sinc") {
		resampling_filter = Magick::SincFilter;
	} else if (filter_name == "hermite") {
		resampling_filter = Magick::HermiteFilter;
	} else if (filter_name == "hanning") {
		resampling_filter = Magick::HanningFilter;
	} else if (filter_name == "hamming") {
		resampling_filter = Magick::HammingFilter;
	} else if (filter_name == "blackman") {
		resampling_filter = Magick::BlackmanFilter;
	} else if (filter_name == "gaussian") {
		resampling_filter = Magick::GaussianFilter;
	} else if (filter_name == "quadratic") {
		resampling_filter = Magick::QuadraticFilter;
	} else if (filter_name == "catrom") {
		resampling_filter = Magick::CatromFilter;
	} else if (filter_name == "mitchell") {
		resampling_filter = Magick::MitchellFilter;
	} else if (filter_name == "lanczos") {
		resampling_filter = Magick::LanczosFilter;
	} else if (filter_name == "bessel") {
		resampling_filter = Magick::BesselFilter;
	} else {
		resampling_filter = Magick::UndefinedFilter;
		resampling_filter_name = "undefined";
	}
	return resampling_filter;
}

void RasterImage::set_num_threads(int num_threads) {
	this->num_threads = num_threads;
}

void RasterImage::clear() {
	if (subset != nullptr) {
		delete subset;
		subset = nullptr;
	}
	set_resampling_filter("");
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
	//! \note Need at least one operation on the image for syncPixels() to work. erase() is not enough.
	subset->roll(1, 0);

	return subset;
}

bool RasterImage::save(const std::filesystem::path &path) {
	if (subset != nullptr) {
		subset->write(path.string());
		return true;
	}
	return false;
}

bool RasterImage::scale_f(float f) {
	if (subset != nullptr) {
		// No scaling needed?
		if (f >= 0.999f && f <= 1.001f)
			return true;

		scaling_factor = f;

		Magick::Geometry geom_orig = subset->size();
		Magick::Geometry geom_new(geom_orig.width() * f, geom_orig.height() * f);
		subset->filterType(resampling_filter);
		subset->resize(geom_new);
		return true;
	}
	return false;
}

bool RasterImage::scale_to(unsigned int size) {
	if (subset != nullptr) {
		Magick::Geometry geom_orig = subset->size();

		if (geom_orig.width() == size && geom_orig.height() == size)
			return true;

		scaling_factor = ((float) size) / ((float) geom_orig.width());

		Magick::Geometry geom_new(size, size);
		subset->filterType(resampling_filter);
		subset->resize(geom_new);
		return true;
	}
	return false;
}

void RasterImage::remap_values(const unsigned char *values, unsigned char max_value) {
	//! \todo Implement support for remapping colors.
	if (subset != nullptr) {
		unsigned int w = subset->columns();
		unsigned int h = subset->rows();
		unsigned int size = w * h;

		Magick::PixelPacket *px = subset->getPixels(0, 0, w, h);

		float src_val, dst_val;

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(px[i]).shade();

			//! \note The last value is reserved for the mapping of invalid values.
			unsigned char idx = (unsigned char) (255 * src_val);
			if (idx > max_value)
				idx = max_value;

			dst_val = values[idx] / 255.0f;
			px[i] = Magick::ColorGray(dst_val);
		}

		subset->syncPixels();
	}
}

bool RasterImage::multiply(float f) {
	if (subset != nullptr) {
		unsigned int w = subset->columns();
		unsigned int h = subset->rows();
		unsigned int size = w * h;

		Magick::PixelPacket *px = subset->getPixels(0, 0, w, h);

		float src_val, dst_val;

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(px[i]).shade();
			px[i] = Magick::ColorGray(src_val * f);
		}

		subset->syncPixels();
		return true;
	}
	return false;
}

int RasterImage::add_layer_to_netcdf(int ncid, const std::filesystem::path &path, const std::string &name_in_netcdf, unsigned int w, unsigned int h, const int *dimids, unsigned char nd, const void *dst_px) {
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

	// Variable attribute for scaling_factor.
	if ((retval = nc_put_att(ncid, varid, "scaling_factor", NC_FLOAT, 1, &scaling_factor)))
		throw NCException("failed to put attribute scaling_factor to " + name_in_netcdf, path, retval);

	// Variable attribute for resampling method.
	if ((retval = nc_put_att_text(ncid, varid, "resampling_filter", resampling_filter_name.size(), resampling_filter_name.c_str())))
		throw NCException("failed to put attribute resampling_filter to " + name_in_netcdf, path, retval);

	// Variable attribute for last modified date-time.
	std::string last_modified_str = datetime_now_str();
	if ((retval = nc_put_att_text(ncid, varid, "last_modified", last_modified_str.size(), last_modified_str.c_str())))
		throw NCException("failed to put attribute last_modified to " + name_in_netcdf, path, retval);

	return varid;
}

bool RasterImage::add_to_netcdf(const std::filesystem::path &path, const std::string &name_in_netcdf) {
	int ncid = 0;
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
			// Global attribute for version number.
			if ((retval = nc_put_att_text(ncid, NC_GLOBAL, "version", strlen(CM_CONVERTER_VERSION_STR), CM_CONVERTER_VERSION_STR)))
				throw NCException("failed to put global attribute version", path, retval);
			// Global attribute for product name.
			if ((retval = nc_put_att_text(ncid, NC_GLOBAL, "product_name", product_name.size(), product_name.c_str())))
				throw NCException("failed to put global attribute product_name", path, retval);
			// Global attribute for overlap.
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

		Magick::PixelPacket *src_px = subset->getPixels(0, 0, w, h);
		unsigned int yw;

		// Store content.
		if (c == 1) {
			if (main_depth > 8) {
				RasterBufferPan<float> dst_px(size);

				for (unsigned int y=0; y<h; y++) {
					yw = y * w;
					for (unsigned int x=0; x<w; x++) {
						dst_px.v[yw + x] = ((float) src_px[yw + x].green) / MaxRGB;
					}
				}
				add_layer_to_netcdf(ncid, path, name_in_netcdf, w, h, dimids, nd, (const void *) dst_px.v);
			} else {
				RasterBufferPan<unsigned char> dst_px(size);

				for (unsigned int y=0; y<h; y++) {
					yw = y * w;
					for (unsigned int x=0; x<w; x++) {
						dst_px.v[yw + x] = (int) (src_px[yw + x].green * 255 / MaxRGB);
					}
				}
				add_layer_to_netcdf(ncid, path, name_in_netcdf, w, h, dimids, nd, (const void *) dst_px.v);
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

			add_layer_to_netcdf(ncid, path, name_in_netcdf + "_R", w, h, dimids, nd, (const void *) dst_px.r);
			add_layer_to_netcdf(ncid, path, name_in_netcdf + "_G", w, h, dimids, nd, (const void *) dst_px.g);
			add_layer_to_netcdf(ncid, path, name_in_netcdf + "_B", w, h, dimids, nd, (const void *) dst_px.b);
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

// Explicit template instantiation:
template class RasterBufferPan<float>;
template class RasterBufferPan<unsigned char>;
template class RasterBufferRGB<unsigned char>;
