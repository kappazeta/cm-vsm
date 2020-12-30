// Tiled loading of a PNG image
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

#include "raster/png_image.hpp"
#include <cstring>


PNG_Image::PNG_Image() {}
PNG_Image::~PNG_Image() {}

bool PNG_Image::load_header(const std::filesystem::path &path) {
	if (subset != nullptr)
		clear();

	subset = new Magick::Image();
	subset->quiet(false);
	subset->ping(path.string());

	main_geometry = subset->size();
	main_depth = subset->depth();

	Magick::ImageType imgtype = subset->type();
	if (imgtype == Magick::GrayscaleType || imgtype == Magick::BilevelType)
		main_num_components = 1;
	else if (imgtype == Magick::TrueColorType)
		main_num_components = 3;

	return true;
}

bool PNG_Image::load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1) {
	if (subset != nullptr)
		clear();

	Magick::Image img(path);
	img.quiet(false);

	Magick::ImageType imgtype = img.type();
	if (imgtype == Magick::BilevelType)
		img.type(Magick::GrayscaleType);

	main_geometry = img.size();
	main_depth = img.depth();

	if (imgtype == Magick::TrueColorType) {
		main_num_components = 3;
		subset = new Magick::Image(Magick::Geometry(da_x1 - da_x0, da_y1 - da_y0), Magick::ColorRGB(0, 0, 0));
	} else {
		main_num_components = 1;
		subset = new Magick::Image(Magick::Geometry(da_x1 - da_x0, da_y1 - da_y0), Magick::ColorGray(0));
	}

	subset->quiet(false);
	subset->type(img.type());
	subset->depth(img.depth());

	Magick::Geometry geom_new(da_x1 - da_x0, da_y1 - da_y0, da_x0, da_y0);
	img.crop(geom_new);

	subset->composite(img, Magick::NorthWestGravity, Magick::CopyCompositeOp);

	return true;
}

