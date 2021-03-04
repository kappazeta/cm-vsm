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

#include "version.hpp"
#include "raster/esa_s2.hpp"
#include "raster/esa_s2_scl_jp2.hpp"
#include "raster/supervisely_raster.hpp"
#include "raster/segmentsai_raster.hpp"
#include "vector/gml.hpp"
#include "vector/cvat_rasterizer.hpp"
#include "vector/supervisely_rasterizer.hpp"
#include "util/text.hpp"
#include "util/geometry.hpp"
#include <openjpeg.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <Magick++.h>
#include <nlohmann/json.hpp>


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
			gc.convert(path_gml, path_xml);
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
	1, // 11 SNOW                     -> CLEAR
	0  // 12 - 255                    -> UNCLASSIFIED
};

int main(int argc, char* argv[]) {
	std::cout << "Vectorization and splitting tool for the KappaZeta Cloudmask project." << std::endl;
	std::cout << " Version: " << CM_CONVERTER_VERSION_STR << std::endl;
	std::cout << "Running with the following dependencies:" << std::endl
		<< " " << nlohmann::json::meta()["name"] << " " << nlohmann::json::meta()["version"]["string"] << std::endl
		<< " OpenJPEG " << opj_version() << std::endl
		<< " MagickLib " << MagickLibVersionText << std::endl
		<< std::endl;

	if (argc < 2) {
		std::cerr << "Usage: " << CM_CONVERTER_NAME_STR
			<< " [-d S2_PATH] [-D CVAT_PATH] [-r CVAT_XML -n NETCDF] [-b BANDS] [-R SUPERVISELY_DIR -t TILENAME -n NETCDF] [-A CVAT_SAI_PATH] [-S TILESIZE [-s SHRINK]]" << std::endl
			<< "\twhere S2_PATH points to the .SAFE directory of an ESA S2 L2A or L1C product." << std::endl
			<< "\tCVAT_PATH points to the .CVAT directory (pre-processed ESA S2 product)." << std::endl
			<< "\tCVAT_XML points to a CVAT annotations.xml file." << std::endl
			<< "\tCVAT_SAI_PATH points to the .CVAT directory with Segments.AI segmentation masks stored in subtiles." << std::endl
			<< "\tSUPERVISELY_DIR points to a directory with the Supervise.ly annotations files." << std::endl
			<< "\tNETCDF points to a NetCDF file to be updated with the rasterized annotations." << std::endl
			<< "\tBANDS is a comma-separated list of bands to process. If omitted, all bands are processed." << std::endl
			<< "\tTILENAME is the name of the tile to pick from the Supervise.ly directory." << std::endl
			<< "\tTILESIZE is the number of pixels per the edge of a square subtile (default: 512)." << std::endl
			<< "\tSHRINK is the factor by which to downscale from the 10 x 10 m^2 S2 bands (default: -1 (original size))." << std::endl;
		return 1;
	}

	//! \note Magick relies on jasper for JP2 files, and jasper is not able to open the ESA S2 JP2 images.
	Magick::InitializeMagick(*argv);

	std::string arg_path_s2_dir, arg_path_cvat_dir, arg_path_rasterize, arg_path_nc, arg_path_cvat_sai_dir, arg_path_supervisely, arg_tilename;
	std::string arg_bands;
	unsigned int tilesize = 512;
	int downscale = -1;
	for (int i=0; i<argc; i++) {
		if (!strncmp(argv[i], "-d", 2))
			arg_path_s2_dir.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-D", 2))
			arg_path_cvat_dir.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-r", 2))
			arg_path_rasterize.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-b", 2))
			arg_bands.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-R", 2))
			arg_path_supervisely.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-n", 2))
			arg_path_nc.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-A", 2))
			arg_path_cvat_sai_dir.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-S", 2))
			tilesize = atoi(argv[i + 1]);
		else if (!strncmp(argv[i], "-s", 2))
			downscale = atoi(argv[i + 1]);
		else if (!strncmp(argv[i], "-t", 2))
			arg_tilename.assign(argv[i + 1]);
	}

	if (arg_path_s2_dir.length() > 0) {
		ESA_S2_Image img;
		ImageOperator img_op;
		std::filesystem::path path_dir_in(arg_path_s2_dir);
		std::filesystem::path path_dir_out(path_dir_in.parent_path().string() + "/" + path_dir_in.stem().string() + ".CVAT");
		std::vector<std::string> bands;

		// All bands, by default.
		if (arg_bands.empty()) {
			bands = std::vector<std::string>(
				&ESA_S2_Image_Operator::data_type_name[0],
				&ESA_S2_Image_Operator::data_type_name[ESA_S2_Image_Operator::DT_COUNT]
			);
		} else {
			bands = split_str(arg_bands, ',');
		}

		img.set_tile_size(tilesize);
		img.set_scl_class_map(new_class_map);
		img.set_downscale_factor(downscale);
		img.process(path_dir_in, path_dir_out, img_op, bands);
	} else if (arg_path_cvat_dir.length() > 0) {
		std::cout << arg_path_cvat_dir << std::endl;

		std::cerr << "ERROR: Processing of pre-processed products is not supported yet." << std::endl;
		return 3;
	}

	if (arg_path_rasterize.length() > 0) {
		CVATRasterizer rasterizer;

		std::filesystem::path path_in(arg_path_rasterize);
		std::filesystem::path path_out_nc(arg_path_nc);

		if (!std::filesystem::exists(path_in)) {
			std::cerr << "ERROR: Vector annotations file " << path_in << " does not exist." << std::endl;
			return 1;
		}
		if (!std::filesystem::exists(path_out_nc)) {
			std::cerr << "ERROR: NetCDF output file " << path_out_nc << " does not exist. Please process the product directory first." << std::endl;
			return 2;
		}

		// Is it an XML file?
		if (endswith(path_in.string(), ".xml")) {
			std::filesystem::path path_out_png(path_in.parent_path().string() + "/" + path_in.stem().string() + ".png");

			rasterizer.convert(path_in, path_out_nc, path_out_png);
		}
	} else if (arg_path_supervisely.length() > 0) {
		SuperviselyRasterizer r;

		std::filesystem::path path_in(arg_path_supervisely);
		std::filesystem::path path_out_nc(arg_path_nc);

		if (std::filesystem::is_directory(path_in)) {
			std::filesystem::path path_out_png(path_in.parent_path().string() + "/supervisely_vector_" + path_in.stem().string() + ".png");

			r.convert(path_in, arg_tilename, path_out_nc, path_out_png);
		} else {
			std::cerr << "Directory " << path_in << " does not exist." << std::endl;
			return 1;
		}
	} else if (arg_path_cvat_sai_dir.length() > 0) {
		SegmentsAIRaster r;

		std::filesystem::path path_in(arg_path_cvat_sai_dir);

		if (std::filesystem::is_directory(path_in)) {
			r.convert(path_in);
		} else {
			std::cerr << "Directory " << path_in << " does not exist." << std::endl;
			return 1;
		}
	}

	return 0;
}

