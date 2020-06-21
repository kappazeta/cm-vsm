#pragma once

#include "raster/raster_image.hpp"

#include <filesystem>


class JP2_Image: public RasterImage {
	public:
		JP2_Image();
		~JP2_Image();
		
		bool load_header(const std::filesystem::path &path);
		
		bool load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1);

		static void error_callback(const char *msg, void *client_data);

		static void warning_callback(const char *msg, void *client_data);

		static void info_callback(const char *msg, void *client_data);
};

