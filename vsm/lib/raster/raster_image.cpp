#include "raster/raster_image.hpp"
#include "util/text.hpp"
#include <png.h>
#include <stdio.h>


RasterImage::RasterImage(): x0(0), y0(0), w(0), h(0), bit_depth(0), num_components(0), color_type(CT_RGB), pixels(nullptr) {}
RasterImage::~RasterImage() {
	clear();
}

void RasterImage::clear() {
	x0 = 0;
	y0 = 0;
	w = 0;
	h = 0;

	if (pixels != nullptr) {
		delete [] pixels;
		pixels = nullptr;
	}
}

std::ostream& operator<<(std::ostream &out, const RasterImage& img) {
	return out << "RasterImage(x0=" << img.x0 << ", y0=" << img.y0 << ", w=" << img.w << ", h=" << img.h << ", d=" << img.bit_depth << ")"; 
}

bool RasterImage::save(const std::filesystem::path &path) {
	if (path.extension() == ".png")
		return save_png(path);
	return false;
}

bool RasterImage::save_png(const std::filesystem::path &path) {
	// Used as reference:
	//  http://zarb.org/~gc/html/libpng.html

	unsigned char **p_rows = nullptr;
	FILE *fp = nullptr;
	bool retval = true;

	try {
		// Open the file for writing binary.
		fp = fopen(path.string().c_str(), "wb");
		if (!fp) {
			std::cerr << "ERROR: LibPNG: Failed to open file " << path << std::endl;
			throw std::exception();
		}

		// Initialize the PNG.
		png_structp p_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!p_png) {
			std::cerr << "ERROR: LibPNG: Failed to create PNG structure for file " << path << std::endl;
			throw std::exception();
		}

		// Create a PNG info structure.
		png_infop p_info = png_create_info_struct(p_png);
		if (!p_info) {
			std::cerr << "ERROR: LibPNG: Failed to create PNG info structure for file " << path << std::endl;
			throw std::exception();
		}

		// Return point for error in png_init_io.
		if (setjmp(png_jmpbuf(p_png))) {
			std::cerr << "ERROR: LibPNG: Failed to initialize IO for " << path << std::endl;
			throw std::exception();
		}

		// Initialize PNG I/O with the file pointer.
		png_init_io(p_png, fp);

		// Return point for error in png_set_IHDR.
		if (setjmp(png_jmpbuf(p_png))) {
			std::cerr << "ERROR: LibPNG: Failed to write header for " << path << std::endl;
			throw std::exception();
		}

		int ct = PNG_COLOR_TYPE_RGB;
		if (color_type == CT_GRAYSCALE)
			ct = PNG_COLOR_TYPE_GRAY;

		// Set PNG header parameters.
		png_set_IHDR(p_png, p_info, this->w, this->h, this->bit_depth, ct,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		// Write the header to file.
		png_write_info(p_png, p_info);

		// Return point for an error in png_write_image.
		if (setjmp(png_jmpbuf(p_png))) {
			std::cerr << "ERROR: LibPNG: Failed to write content for " << path << std::endl;
			throw std::exception();
		}

		// Generate a list of row pointers.
		p_rows = new unsigned char *[h];
		for (int y=0; y<h; y++)
			p_rows[y] = &pixels[y * w * num_components];
		// Write the rows to file.
		png_write_image(p_png, p_rows);

		// Return point for an error in png_write_end.
		if (setjmp(png_jmpbuf(p_png))) {
			std::cerr << "ERROR: LibPNG: Failed to finish writing of " << path << std::endl;
			throw std::exception();
		}

		// Finish writing the file.
		png_write_end(p_png, nullptr);
	} catch(std::exception &e) {
		retval = false;
	}

	// Free allocated memory (if any).
	if (fp)
		fclose(fp);
	if (p_rows)
		delete [] p_rows;
	return retval;
}

