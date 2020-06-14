#pragma once

#include "raster/esa_s2_tci_jp2.hpp"

#include <filesystem>

//! \todo Base it on JP2_Image.

class ESA_S2_SCL_JP2_Image: public ESA_S2_TCI_JP2_Image {
	public:
		ESA_S2_SCL_JP2_Image();
		~ESA_S2_SCL_JP2_Image();
		
		static const std::string class_names[12];
		unsigned char class_map[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
};

std::ostream &operator<<(std::ostream &out, const ESA_S2_SCL_JP2_Image &img);

