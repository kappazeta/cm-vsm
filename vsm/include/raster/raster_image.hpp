#pragma once

#include <iostream>
#include <filesystem>


class RasterImage {
	public:
		RasterImage();
		~RasterImage();

		typedef enum color_type_t {
			CT_GRAYSCALE,
			CT_RGB
		};
		
		virtual bool load(const std::filesystem::path &path) {}
		bool save(const std::filesystem::path &path);
		void clear();

		unsigned int x0, y0;
		unsigned int w, h;
		unsigned char num_components;
		unsigned char bit_depth;
		color_type_t color_type;
		unsigned char *pixels;

		void remap_values(const unsigned char *values);

	protected:
		bool save_png(const std::filesystem::path &path);
};

std::ostream &operator<<(std::ostream &out, const RasterImage &img);

