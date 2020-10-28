// ESA S2 product converter for cloud mask labeling and processing
//
// Copyright 2020 KappaZeta Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "raster/esa_s2.hpp"
#include "raster/esa_s2_scl_jp2.hpp"
#include "vector/gml.hpp"
#include "vector/cvat_rasterizer.hpp"
#include "util/text.hpp"
#include "util/geometry.hpp"
#include <openjpeg.h>
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
	4, // 8  CLOUD_MEDIUM_PROBABILITY -> CLOUD
	4, // 9  CLOUD_HIGH_PROBABILITY   -> CLOUD
	3, // 10 THIN_CIRRUS              -> SEMI_TRANSPARENT_CLOUD
	1  // 11 SNOW                     -> CLEAR
};

int main(int argc, char* argv[]) {
	std::cout << "Vectorization and splitting tool for CVAT." << std::endl;
	std::cout << "Running with the following dependencies:" << std::endl
		<< " GDAL " << GDAL_RELEASE_NAME << " (" << GDAL_RELEASE_DATE << ")" << std::endl
		<< " OpenJPEG " << opj_version() << std::endl
		<< " MagickLib " << MagickLibVersionText << std::endl
		<< std::endl;

	if (argc < 2) {
		std::cerr << "Usage: cvat-vsm [-d PATH][-r CVAT_XML [-n NETCDF]]" << std::endl
			<< "\twhere PATH points to the .SAFE directory of an ESA S2 L2A product." << std::endl
			<< "\tCVAT_XML to an annotations vector layer which is to be rasterized." << std::endl
			<< "\tNETCDF to a NetCDF file to be updated with the rasterized annotations." << std::endl;
		return 1;
	}

	//! \note Magick relies on jasper for JP2 files, and jasper is not able to open the ESA S2 JP2 images.
	Magick::InitializeMagick(*argv);

	std::string arg_path_dir, arg_path_rasterize, arg_path_nc;
	for (int i=0; i<argc; i++) {
		if (!strncmp(argv[i], "-d", 2))
			arg_path_dir.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-r", 2))
			arg_path_rasterize.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-n", 2))
			arg_path_nc.assign(argv[i + 1]);
	}

	if (arg_path_dir.length() > 0) {
		ESA_S2_Image img;
		ImageOperator img_op;
		std::filesystem::path path_dir_in(arg_path_dir);
		std::filesystem::path path_dir_out(path_dir_in.parent_path().string() + "/" + path_dir_in.stem().string() + ".CVAT");

		img.set_scl_class_map(new_class_map);
		img.process(path_dir_in, path_dir_out, img_op);
	}

	if (arg_path_rasterize.length() > 0) {
		std::cout << arg_path_rasterize << std::endl;
		CVATRasterizer rasterizer;

		std::filesystem::path path_in(arg_path_rasterize);
		std::filesystem::path path_out_nc(arg_path_nc);
		std::filesystem::path path_out_png(path_in.parent_path().string() + "/" + path_in.stem().string() + ".png");

		if (!std::filesystem::exists(path_in)) {
			std::cerr << "ERROR: Vector annotations file " << path_in << " does not exist." << std::endl;
			return 1;
		}
		if (!std::filesystem::exists(path_out_nc)) {
			std::cerr << "ERROR: NetCDF output file " << path_out_nc << " does not exist. Please process the product directory first." << std::endl;
			return 2;
		}

		rasterizer.convert(path_in, path_out_nc, path_out_png);
	}

	return 0;
}

