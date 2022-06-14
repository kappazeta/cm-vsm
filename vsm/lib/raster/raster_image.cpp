// Generic raster image
//
// Copyright 2021 - 2022 KappaZeta Ltd.
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
	} else if (filter_name == "cubic") {
		resampling_filter = Magick::CubicFilter;
	} else if (filter_name == "sinc") {
		resampling_filter = Magick::SincFilter;
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

// Explicit template instantiation:
template class RasterBufferPan<float>;
template class RasterBufferPan<unsigned char>;
template class RasterBufferRGB<unsigned char>;
