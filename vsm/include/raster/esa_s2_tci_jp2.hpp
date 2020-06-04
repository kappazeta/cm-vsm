#pragma once

#include "raster/raster_image.hpp"

#include <filesystem>


class ESA_S2_TCI_JP2_Image: public RasterImage {
	public:
		ESA_S2_TCI_JP2_Image();
		~ESA_S2_TCI_JP2_Image();
		
		bool load_header(const std::filesystem::path &path);
		
		bool load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1);

		static void error_callback(const char *msg, void *client_data);

		static void warning_callback(const char *msg, void *client_data);

		static void info_callback(const char *msg, void *client_data);
};

std::ostream &operator<<(std::ostream &out, const ESA_S2_TCI_JP2_Image &img);

