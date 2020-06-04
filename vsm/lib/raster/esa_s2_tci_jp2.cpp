#include "raster/esa_s2_tci_jp2.hpp"
#include <openjpeg.h>

#define JP2_CFMT	1


ESA_S2_TCI_JP2_Image::ESA_S2_TCI_JP2_Image() {}
ESA_S2_TCI_JP2_Image::~ESA_S2_TCI_JP2_Image() {}


std::ostream& operator<<(std::ostream &out, const ESA_S2_TCI_JP2_Image& img) {
	return out << "ESA_S2_TCI_JP2_Image(x0=" << img.x0 << ", y0=" << img.y0 << ", w=" << img.w << ", h=" << img.h << ", d=" << img.bit_depth << ")"; 
}

void ESA_S2_TCI_JP2_Image::error_callback(const char *msg, void *client_data) {
	std::cerr << "ERROR: OpenJPEG: " << msg;
}

void ESA_S2_TCI_JP2_Image::warning_callback(const char *msg, void *client_data) {
	std::cout << "WARN: OpenJPEG: " << msg;
}

void ESA_S2_TCI_JP2_Image::info_callback(const char *msg, void *client_data) {
	std::cout << "INFO: OpenJPEG: " << msg;
}

bool ESA_S2_TCI_JP2_Image::load_header(const std::filesystem::path &path) {
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

		opj_set_info_handler(l_codec, ESA_S2_TCI_JP2_Image::info_callback, nullptr);
		opj_set_warning_handler(l_codec, ESA_S2_TCI_JP2_Image::warning_callback, nullptr);
		opj_set_error_handler(l_codec, ESA_S2_TCI_JP2_Image::error_callback, nullptr);

		if (!opj_setup_decoder(l_codec, &l_param)) {
			std::cerr << "ERROR: OpenJPEG: Failed to setup decoder for " << path << std::endl;
			throw std::exception();
		}

		// Read file header with image size, number of components, etc.
		if (!opj_read_header(l_stream, l_codec, &l_image)) {
			std::cerr << "ERROR: OpenJPEG: Failed to read header from " << path << std::endl;
			throw std::exception();
		}

		std::cout << "INFO: Image size: " << l_image->x0 << ", " << l_image->y0 << ", " << l_image->x1 << ", " << l_image->y1 << std::endl;
		std::cout << "INFO: Number of pixel components: " << l_image->numcomps << std::endl;

		x0 = l_image->x0;
		y0 = l_image->y0;
		w = l_image->x1 - x0;
		h = l_image->y1 - y0;
		bit_depth = 8;
		num_components = l_image->numcomps;

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

bool ESA_S2_TCI_JP2_Image::load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1) {
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
		opj_set_warning_handler(l_codec, ESA_S2_TCI_JP2_Image::warning_callback, nullptr);
		opj_set_error_handler(l_codec, ESA_S2_TCI_JP2_Image::error_callback, nullptr);

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

		// Process the whole image.
		if (!opj_set_decode_area(l_codec, l_image, da_x0, da_y0, da_x1, da_y1)) {
			std::cerr << "ERROR: OpenJPEG: Failed to set decoded area " <<
				da_x0 << ", " << da_y0 << ", " << da_x1 << ", " << da_y1 << " for " << path << std::endl;
			throw std::exception();
		}

		// Allocate for the pixels.
		w = da_x1 - da_x0;
		h = da_y1 - da_y0;
		bit_depth = 8;
		num_components = l_image->numcomps;
		if (pixels != nullptr)
			delete [] pixels;
		pixels = new unsigned char[w * h * num_components];

		if (num_components == 1)
			color_type = CT_GRAYSCALE;

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

				//! \todo Vectorize this?

				unsigned int l_ctile_width = l_ctile_x1 - l_ctile_x0;
				unsigned int l_ctile_height = l_ctile_y1 - l_ctile_y0;
				unsigned int l_ctile_size = l_ctile_width * l_ctile_height;
				unsigned int gp_x, gp_y;
				unsigned int tlpi, sspi;
				// Iterate over components in the tile.
				for (unsigned char c=0; c<l_n_components; c++) {
					// Iterate over rows in the tile.
					for (unsigned int y=0; y<l_ctile_height; y++) {
						// Iterate on columns in the tile.
						for (unsigned int x=0; x<l_ctile_width; x++) {
							// Tile-local pixel index
							tlpi = c * l_ctile_size + y * l_ctile_width + x;
							// Global pixel coordinates
							gp_x = l_ctile_x0 + x;
							gp_y = l_ctile_y0 + y;

							// Verify that the pixel is within our decode area.
							if (gp_x >= da_x0 && gp_y >= da_y0 && gp_x < da_x1 && gp_y < da_y1) {
								// Subset pixel index
								sspi = num_components * ((gp_y - da_y0) * w + gp_x - da_x0);

								pixels[sspi + c] = l_data[tlpi];
							}
						}
					}
				}
			}
		}

		// Finish the stream.
		if (!opj_end_decompress(l_codec, l_stream)) {
			std::cerr << "ERROR: OpenJPEG: Failed to end decompression of " << path << std::endl;
			throw std::exception();
		}

	} catch(std::exception &e) {
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


