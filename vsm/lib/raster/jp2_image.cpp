// Tiled loading of a JP2 image
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

#include "raster/jp2_image.hpp"
#include <openjpeg.h>
#include <cstring>

#define JP2_CFMT	1


JP2_Image::JP2_Image() {}
JP2_Image::~JP2_Image() {}

void JP2_Image::error_callback(const char *msg, void *client_data) {
	std::cerr << "ERROR: OpenJPEG: " << msg;
}

void JP2_Image::warning_callback(const char *msg, void *client_data) {
	std::cout << "WARN: OpenJPEG: " << msg;
}

void JP2_Image::info_callback(const char *msg, void *client_data) {
	std::cout << "INFO: OpenJPEG: " << msg;
}

bool JP2_Image::load_header(const std::filesystem::path &path) {
	// Used as reference:
	//  https://github.com/uclouvain/openjpeg/blob/master/tests/test_tile_decoder.c

	opj_dparameters_t l_param;
	opj_codec_t* l_codec = nullptr;
	opj_image_t* l_image = nullptr;

	opj_stream_t* l_stream = nullptr;

	bool retval = true;

	try {
		// Create a stream from the file.
		l_stream = opj_stream_create_default_file_stream(path.string().c_str(), OPJ_TRUE);
		if (!l_stream) {
			std::cerr << "ERROR: OpenJPEG: Failed to create stream from " << path << std::endl;
			throw std::exception();
		}

		// Initialize the decoder.
		opj_set_default_decoder_parameters(&l_param);
		l_param.decod_format = JP2_CFMT;

		l_codec = opj_create_decompress(OPJ_CODEC_JP2);

		opj_set_info_handler(l_codec, JP2_Image::info_callback, nullptr);
		opj_set_warning_handler(l_codec, JP2_Image::warning_callback, nullptr);
		opj_set_error_handler(l_codec, JP2_Image::error_callback, nullptr);

		if (!opj_setup_decoder(l_codec, &l_param)) {
			std::cerr << "ERROR: OpenJPEG: Failed to setup decoder for " << path << std::endl;
			throw std::exception();
		}

		// Read file header with image size, number of components, etc.
		if (!opj_read_header(l_stream, l_codec, &l_image)) {
			std::cerr << "ERROR: OpenJPEG: Failed to read header from " << path << std::endl;
			throw std::exception();
		}

		if (subset != nullptr)
			clear();

		main_geometry.xOff(l_image->x0);
		main_geometry.yOff(l_image->y0);
		main_geometry.width(l_image->x1 - l_image->x0);
		main_geometry.height(l_image->y1 - l_image->y0);

		if (l_image->comps->prec <= 8)
			main_depth = 8;
		else
			main_depth = 16;

		main_num_components = l_image->numcomps;
		
		std::cout << "INFO: Image size: " << l_image->x0 << ", " << l_image->y0 << ", " << l_image->x1 << ", " << l_image->y1 << std::endl;
		std::cout << "INFO: Number of pixel components: " << l_image->numcomps << " with depth: " << (int) main_depth << std::endl;

	} catch(std::exception &e) {
		retval = false;
	}

	// Free the allocated memory (if any).
	if (l_stream != nullptr)
		opj_stream_destroy(l_stream);
	if (l_codec != nullptr)
		opj_destroy_codec(l_codec);
	if (l_image != nullptr)
		opj_image_destroy(l_image);
	return retval;
}

bool JP2_Image::load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1) {
	// Used as reference:
	//  https://github.com/uclouvain/openjpeg/blob/master/tests/test_tile_decoder.c
	//  https://web.archive.org/web/20180423091842/http://www.equasys.de/colorconversion.html

	opj_dparameters_t l_param;
	OPJ_UINT32 l_n_components = 0;
	opj_codec_t* l_codec = nullptr;
	opj_image_t* l_image = nullptr;

	opj_stream_t* l_stream = nullptr;
	OPJ_BYTE* l_data = (OPJ_BYTE *) malloc(10 * 1024 * 1024);
	OPJ_UINT32 l_max_data_size = 10 * 1024 * 1024;
	OPJ_UINT32 l_data_size;
	OPJ_UINT32 l_tile_index;
	OPJ_INT32 l_ctile_x0, l_ctile_y0, l_ctile_x1, l_ctile_y1;

	bool retval = true;

	try {
		// Create a stream from the file.
		l_stream = opj_stream_create_default_file_stream(path.string().c_str(), OPJ_TRUE);
		if (!l_stream) {
			std::cerr << "ERROR: OpenJPEG: Failed to create stream from " << path << std::endl;
			throw std::exception();
		}

		// Initialize the decoder.
		opj_set_default_decoder_parameters(&l_param);
		l_param.decod_format = JP2_CFMT;

		l_codec = opj_create_decompress(OPJ_CODEC_JP2);

		// Avoid setting the info handler, to reduce spam.
		opj_set_warning_handler(l_codec, JP2_Image::warning_callback, nullptr);
		opj_set_error_handler(l_codec, JP2_Image::error_callback, nullptr);

		if (!opj_setup_decoder(l_codec, &l_param)) {
			std::cerr << "ERROR: OpenJPEG: Failed to setup decoder for " << path << std::endl;
			throw std::exception();
		}

		// Read file header with image size, number of components, etc.
		if (!opj_read_header(l_stream, l_codec, &l_image)) {
			std::cerr << "ERROR: OpenJPEG: Failed to read header from " << path << std::endl;
			throw std::exception();
		}

		//! \note ESA S2 JP2 headers lack colorspace info. It seems that pixels are stored as RGB instead of YUV.

		int da_x1_clamped = da_x1, da_y1_clamped = da_y1;
		int w = da_x1 - da_x0;
		int h = da_y1 - da_y0;

		if (da_x1 > da_x0 + w)
			da_x1_clamped = da_x0 + w;
		if (da_y1 > da_y0 + h)
			da_y1_clamped = da_y0 + h;

		// Process the whole image.
		if (!opj_set_decode_area(l_codec, l_image, da_x0, da_y0, da_x1_clamped, da_y1_clamped)) {
			std::cerr << "ERROR: OpenJPEG: Failed to set decoded area " <<
				da_x0 << ", " << da_y0 << ", " << da_x1 << ", " << da_y1 << " for " << path << std::endl;
			throw std::exception();
		}

		if (subset != nullptr)
			clear();

		subset = new Magick::Image();
		subset->quiet(false);
		subset->size(Magick::Geometry(w, h));
		subset->depth((int) main_depth);
		subset->endian(Magick::LSBEndian);
		if (main_num_components == 1) {
			subset->type(Magick::GrayscaleType);
			subset->backgroundColor(Magick::ColorGray(0));
		} else if (main_num_components == 3) {
			subset->type(Magick::TrueColorType);
			subset->backgroundColor(Magick::ColorRGB(0, 0, 0));
		}

		OPJ_BYTE l_val = 0;
		OPJ_BOOL l_continue = OPJ_TRUE;
		while (l_continue) {
			// Read tile header.
			if (!opj_read_tile_header(
						l_codec, l_stream, &l_tile_index, &l_data_size,
						&l_ctile_x0, &l_ctile_y0, &l_ctile_x1, &l_ctile_y1,
						&l_n_components, &l_continue)) {
				throw std::exception();
			}

			// Process until we run out of tiles.
			if (l_continue) {
				// We have a tile which is larger than the buffer which we have allocated?
				if (l_data_size > l_max_data_size) {
					OPJ_BYTE *l_new_data = (OPJ_BYTE *) realloc(l_data, l_data_size);
					if (!l_new_data) {
						std::cerr << "ERROR: OpenJPEG: Failed to reallocate from "
							<< l_max_data_size << " to " << l_data_size << std::endl;
						throw std::exception();
					}
					l_data = l_new_data;
					l_max_data_size = l_data_size;
				}

				// Read and decompress tile contents.
				if (!opj_decode_tile_data(l_codec, l_tile_index, l_data, l_data_size, l_stream)) {
					std::cerr << "ERROR: OpenJPEG: Failed to decode a tile from " << path << std::endl;
					throw std::exception();
				}

				// OpenJPEG seems to have adjacent sub-component tiles.

				unsigned int l_ctile_width = l_ctile_x1 - l_ctile_x0;
				unsigned int l_ctile_height = l_ctile_y1 - l_ctile_y0;
				unsigned int l_ctile_size = l_ctile_width * l_ctile_height;
				unsigned int l_ctile_size_b = l_ctile_width * l_ctile_height * main_depth / 8;

				Magick::Image img;
				img.quiet(false);
				img.size(Magick::Geometry(l_ctile_width, l_ctile_height));
				img.depth(main_depth);
				img.type(subset->type());
				img.endian(Magick::LSBEndian);
				img.backgroundColor(subset->backgroundColor());

				Magick::StorageType px_storage;
				if (main_depth <= 8)
					px_storage = Magick::CharPixel;
				else if (main_depth <= 16)
					px_storage = Magick::ShortPixel;

				// Iterate over components in the tile and
				// blit them on the tile image.
				for (unsigned char c=0; c<l_n_components; c++) {
					Magick::Image img_c(img);
					img_c.quiet(false);
					img_c.depth(main_depth);
					img_c.type(img.type());

					img_c.endian(Magick::MSBEndian);
					img_c.read(l_ctile_width, l_ctile_height, "I", px_storage, &l_data[c * l_ctile_size_b]);
					img_c.endian(Magick::LSBEndian);

					if (l_n_components > 1) {
						if (c == 0)
							img.composite(img_c, 0, 0, Magick::CopyRedCompositeOp);
						else if (c == 1)
							img.composite(img_c, 0, 0, Magick::CopyGreenCompositeOp);
						else if (c == 2)
							img.composite(img_c, 0, 0, Magick::CopyBlueCompositeOp);
					} else {
						img = img_c;
					}
				}

				// Blit the tile on the subset image.
				subset->composite(img, l_ctile_x0 - da_x0, l_ctile_y0 - da_y0, Magick::AtopCompositeOp);
			}
		}

		// Finish the stream.
		if (!opj_end_decompress(l_codec, l_stream)) {
			std::cerr << "ERROR: OpenJPEG: Failed to end decompression of " << path << std::endl;
			throw std::exception();
		}

	} catch(std::exception &e) {
		std::cerr << e.what() << std::endl;
		retval = false;
	}

	// Free the allocated memory (if any).
	if (l_data != nullptr)
		free(l_data);
	if (l_stream != nullptr)
		opj_stream_destroy(l_stream);
	if (l_codec != nullptr)
		opj_destroy_codec(l_codec);
	if (l_image != nullptr)
		opj_image_destroy(l_image);
	return retval;
}


