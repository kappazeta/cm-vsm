#pragma once

#include "raster/jp2_image.hpp"
#include <filesystem>


class ESA_S2_Band_JP2_Image: public JP2_Image {
	public:
		ESA_S2_Band_JP2_Image();
		~ESA_S2_Band_JP2_Image();
};

