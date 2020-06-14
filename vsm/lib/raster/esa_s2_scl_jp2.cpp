#include "raster/esa_s2_scl_jp2.hpp"


ESA_S2_SCL_JP2_Image::ESA_S2_SCL_JP2_Image() {}
ESA_S2_SCL_JP2_Image::~ESA_S2_SCL_JP2_Image() {}


std::ostream& operator<<(std::ostream &out, const ESA_S2_SCL_JP2_Image& img) {
	return out << "ESA_S2_SCL_JP2_Image(x0=" << img.x0 << ", y0=" << img.y0 << ", w=" << img.w << ", h=" << img.h << ", d=" << img.bit_depth << ")"; 
}

// Used as reference:
//  https://sentinel.esa.int/web/sentinel/technical-guides/sentinel-2-msi/level-2a/algorithm
const std::string ESA_S2_SCL_JP2_Image::class_names[] = {
	"NO_DATA",                  // 0
	"SATURATED_OR_DEFECTIVE",   // 1
	"DARK_AREA_PIXELS",         // 2
	"CLOUD_SHADOWS",            // 3
	"VEGETATION",               // 4
	"NOT_VEGETATED",            // 5
	"WATER",                    // 6
	"UNCLASSIFIED",             // 7
	"CLOUD_MEDIUM_PROBABILITY", // 8
	"CLOUD_HIGH_PROBABILITY",   // 9
	"THIN_CIRRUS",              // 10
	"SNOW"                      // 11
};

