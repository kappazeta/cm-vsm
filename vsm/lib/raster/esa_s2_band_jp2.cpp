#include "raster/esa_s2_band_jp2.hpp"


ESA_S2_Band_JP2_Image::ESA_S2_Band_JP2_Image() {}
ESA_S2_Band_JP2_Image::~ESA_S2_Band_JP2_Image() {}


std::ostream& operator<<(std::ostream &out, const ESA_S2_Band_JP2_Image& img) {
	return out << "ESA_S2_Band_JP2_Image(x0=" << img.x0 << ", y0=" << img.y0 << ", w=" << img.w << ", h=" << img.h << ", d=" << img.bit_depth << ")"; 
}

