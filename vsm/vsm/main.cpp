#include "raster/esa_s2.hpp"
#include "raster/esa_s2_scl_jp2.hpp"
#include "vector/gml.hpp"
#include "util/text.hpp"
#include "util/geometry.hpp"
#include <openjpeg.h>
#include <png.h>
#include <gdal.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <Magick++.h>


class ImageOperator: public ESA_S2_Image_Operator {
public:
	bool operator()(const std::filesystem::path &path, data_type_t type) {
		int retval = 0;

		if (type == DT_SCL) {
			std::ostringstream cmd;
			std::string path_gml = path.parent_path().string() + "/" + path.stem().string() + ".GML";
			cmd << "gdal_polygonize.py -8 " << path.string() << " -f GML " << path_gml;

			std::cout << cmd.str() << std::endl;
			retval = system(cmd.str().c_str());
			if (retval != 0)
				return false;

			GMLConverter gc;

			std::vector<std::string> classes {
				"UNCLASSIFIED",
				"CLEAR",
				"CLOUD_SHADOW",
				"CLOUD",
				"SEMI_TRANSPARENT_CLOUD"
			};
			std::vector<int> include_classes{1, 2, 3, 4};
			gc.set_classes(classes, include_classes);
			gc.set_multiplier(2);

			std::string path_xml = path.parent_path().string() + "/" + path.stem().string() + ".xml";
			gc.convert(path_gml, path_xml, "CVAT");
		}
		return true;
	}
};

unsigned char new_class_map[] = {
	0, // 0  NO_DATA                  -> UNCLASSIFIED
	0, // 1  SATURATED_OR_DEFECTIVE   -> UNCLASSIFIED
	1, // 2  DARK_AREA_PIXELS         -> CLEAR
	2, // 3  CLOUD_SHADOWS            -> CLOUD_SHADOW
	1, // 4  VEGETATION               -> CLEAR
	1, // 5  NOT_VEGETATED            -> CLEAR
	1, // 6  WATER                    -> CLEAR
	0, // 7  UNCLASSIFIED             -> UNCLASSIFIED
	3, // 8  CLOUD_MEDIUM_PROBABILITY -> CLOUD
	3, // 9  CLOUD_HIGH_PROBABILITY   -> CLOUD
	4, // 10 THIN_CIRRUS              -> SEMI_TRANSPARENT_CLOUD
	1  // 11 SNOW                     -> CLEAR
};

int main(int argc, char* argv[]) {
	std::cout << "Vectorization and splitting tool for CVAT." << std::endl;
	std::cout << "Running with the following dependencies:" << std::endl
		<< " GDAL " << GDAL_RELEASE_NAME << " (" << GDAL_RELEASE_DATE << ")" << std::endl
		<< " OpenJPEG " << opj_version() << std::endl
		<< PNG_HEADER_VERSION_STRING
		<< " MagickLib " << MagickLibVersionText << std::endl
		<< std::endl;

	if (argc < 2) {
		std::cerr << "Usage: cvat-vsm PATH" << std::endl << "\twhere PATH points to the .SAFE directory of an ESA S2 L2A product." << std::endl;
		return 1;
	}

	//! \note Magick relies on jasper for JP2 files, and jasper is not able to open the ESA S2 JP2 images.
	Magick::InitializeMagick(*argv);

	ESA_S2_Image img;
	ImageOperator img_op;
	std::filesystem::path path_dir_in(argv[1]);
	std::filesystem::path path_dir_out(path_dir_in.parent_path().string() + "/" + path_dir_in.stem().string() + ".CVAT");

	img.set_scl_class_map(new_class_map);
	img.process(path_dir_in, path_dir_out, img_op);

	return 0;
}

