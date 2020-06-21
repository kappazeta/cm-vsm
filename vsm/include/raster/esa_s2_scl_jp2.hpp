#pragma once

#include "raster/jp2_image.hpp"

#include <filesystem>


class ESA_S2_SCL_JP2_Image: public JP2_Image {
	public:
		ESA_S2_SCL_JP2_Image();
		~ESA_S2_SCL_JP2_Image();
		
		static const std::string class_names[12];
		unsigned char class_map[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
};

