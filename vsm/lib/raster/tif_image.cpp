// Tiled loading of a TIF image
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

#include "raster/tif_image.hpp"
#include <cstring>
#include "tiffio.h"

TIF_Image::TIF_Image(): num_tiff_channels(0) {}
TIF_Image::~TIF_Image() {}

bool TIF_Image::load_header(const std::filesystem::path &path) {
	if (subset != nullptr)
		clear();

	TIFFSetWarningHandler(NULL);
	TIFF *ptif = TIFFOpen(path.c_str(), "r");

	unsigned short n_chan = 0;
	TIFFGetField(ptif, TIFFTAG_SAMPLESPERPIXEL, &n_chan);
	num_tiff_channels = n_chan;
	// If the TIFF file contains just 1 or 3 channels, then we can store it as a single grayscale or RGB image.
	// Otherwise treat each channel as a separate grayscale image.
	main_num_components = 1;
	if (num_tiff_channels == 1 || num_tiff_channels == 3)
		main_num_components = n_chan;

	unsigned short depth = 0;
	TIFFGetField(ptif, TIFFTAG_BITSPERSAMPLE, &depth);
	main_depth = depth;

	uint w = 0, h = 0;
	TIFFGetField(ptif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(ptif, TIFFTAG_IMAGELENGTH, &h);
	main_geometry.width(w);
	main_geometry.height(h);

	TIFFClose(ptif);

	return true;
}

bool TIF_Image::load_subset(const std::filesystem::path &path, unsigned int da_x0, unsigned int da_y0, unsigned int da_x1, unsigned int da_y1) {
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

bool TIF_Image::load_subset_channel(const std::filesystem::path &path, unsigned int da_x0, unsigned int da_y0, unsigned int da_x1, unsigned int da_y1, unsigned int channel) {
	tsize_t retval = -1;
	unsigned int blocksize = 0;
	unsigned int tile_w = 0, tile_h = 0;
	unsigned int da_x1c = da_x1;
	unsigned int da_y1c = da_y1;
	unsigned int x, y, yi;
	unsigned int sx, sy;
	unsigned int rows_per_strip = 0;
	unsigned short planar_cfg = 0;
	float dst_val;

	// Make sure that we won't ask for out of range pixels from libtiff.
	if (channel > num_tiff_channels)
		return false;
	if (da_x0 > main_geometry.width() || da_y0 > main_geometry.height())
		return false;
	if (da_x1 > main_geometry.width())
		da_x1c = main_geometry.width();
	if (da_y1 > main_geometry.height())
		da_y1c = main_geometry.height();

	Magick::Geometry geom(main_geometry);
	create_grayscale(Magick::Geometry(da_x1 - da_x0, da_y1 - da_y0), main_depth, 0);
	main_geometry = geom;

	Magick::PixelPacket *px = subset->getPixels(0, 0, da_x1 - da_x0, da_y1 - da_y0);

	tdata_t pbuf = nullptr;
	TIFF *ptif = TIFFOpen(path.c_str(), "r");
	if (ptif) {
		TIFFGetField(ptif, TIFFTAG_PLANARCONFIG, &planar_cfg);

		if (TIFFIsTiled(ptif)) {
			TIFFGetField(ptif, TIFFTAG_TILEWIDTH, &tile_w);
			TIFFGetField(ptif, TIFFTAG_TILELENGTH, &tile_h);
			unsigned int da_x0t = da_x0 / tile_w;
			unsigned int da_y0t = da_y0 / tile_h;
			unsigned int da_x1t = da_x1 / tile_w;
			unsigned int da_y1t = da_y1 / tile_h;

			if (da_x1 % tile_w > 0)
				da_x1t++;
			if (da_y1 % tile_h > 0)
				da_y1t++;
			da_x0t *= tile_w;
			da_y0t *= tile_h;
			da_x1t *= tile_w;
			da_y1t *= tile_h;


			blocksize = TIFFTileSize(ptif);
			pbuf = _TIFFmalloc(blocksize);
			for (y = da_y0t; y < da_y1t; y += tile_h) {
				for (x = da_x0t; x < da_x1t; x += tile_w) {
					retval = TIFFReadTile(ptif, pbuf, x, y, 0, channel);

					//! \todo Implement blitting to Magick::Image
				}
			}
		} else {
			TIFFGetField(ptif, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
			unsigned int num_strips = TIFFNumberOfStrips(ptif);

			tile_w = main_geometry.width();
			tile_h = rows_per_strip;

			blocksize = TIFFStripSize(ptif);
			pbuf = _TIFFmalloc(blocksize);
			for (y = da_y0; y < da_y1c; y += rows_per_strip) {
				if (planar_cfg == PLANARCONFIG_CONTIG) {
					retval = TIFFReadEncodedStrip(ptif, y / rows_per_strip, pbuf, (tsize_t) -1);
				} else if (planar_cfg == PLANARCONFIG_SEPARATE) {
					retval = TIFFReadEncodedStrip(ptif, channel * tile_h + y / rows_per_strip, pbuf, (tsize_t) -1);
				}

				if (main_depth == 32) {
					if (planar_cfg == PLANARCONFIG_CONTIG) {
						for (yi = 0; yi < rows_per_strip; yi++) {
							for (x = da_x0; x < da_x1c; x++) {
								dst_val = ((float *) pbuf)[(x + yi * tile_w) * num_tiff_channels + channel];
								if (dst_val < 0)
									dst_val = 0.0f;
								px[x - da_x0 + (y + yi - da_y0) * (da_x1 - da_x0)] = Magick::ColorGray(dst_val);
							}
						}
					} else if (planar_cfg == PLANARCONFIG_SEPARATE) {
						for (yi = 0; yi < rows_per_strip; yi++) {
							for (x = da_x0; x < da_x1c; x++) {
								dst_val = ((float *) pbuf)[x + yi * tile_w];
								if (dst_val < 0)
									dst_val = 0.0f;
								px[x - da_x0 + (y + yi - da_y0) * (da_x1 - da_x0)] = Magick::ColorGray(dst_val);
							}
						}
					}
				}
			}
		}

		subset->syncPixels();

		_TIFFfree(pbuf);
		TIFFClose(ptif);
	}

	return true;
}

