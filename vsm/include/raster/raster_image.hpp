#pragma once

#include <iostream>
#include <filesystem>
#include <Magick++.h>


class RasterImage {
	public:
		RasterImage();
		~RasterImage();

		virtual bool load(const std::filesystem::path &path) {}
		bool save(const std::filesystem::path &path);
		void clear();

		Magick::Image *subset;

		Magick::Geometry main_geometry;
		unsigned char main_depth;
		unsigned char main_num_components;

		bool scale(float f, bool point_filter);
		void remap_values(const unsigned char *values);
};

std::ostream &operator<<(std::ostream &out, const RasterImage &img);

