#pragma once

#include "raster/esa_s2_tci_jp2.hpp"

#include <filesystem>

//! \todo Base it on JP2_Image.

class ESA_S2_Band_JP2_Image: public ESA_S2_TCI_JP2_Image {
	public:
		ESA_S2_Band_JP2_Image();
		~ESA_S2_Band_JP2_Image();
};

std::ostream &operator<<(std::ostream &out, const ESA_S2_Band_JP2_Image &img);

