// ESA S2 product converter for cloud mask labeling and processing
//
// Copyright 2021 - 2022 KappaZeta Ltd.
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
#include <gdal.h>


unsigned char new_class_map[] = {
	5, // 0  NO_DATA                  -> UNCLASSIFIED
	5, // 1  SATURATED_OR_DEFECTIVE   -> UNCLASSIFIED
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
	5  // 12 - 255                    -> UNCLASSIFIED
};

int main(int argc, char* argv[]) {
	std::cout << "Vectorization and splitting tool for the KappaZeta Cloudmask project." << std::endl;
	std::cout << " Version: " << CM_CONVERTER_VERSION_STR << std::endl;
	std::cout << "Built with the following dependencies:" << std::endl
		<< " MagickLib " << MagickLibVersionText << std::endl
		<< " GDAL " << GDAL_RELEASE_NAME << std::endl;
	std::cout << "Running with the following dependencies:" << std::endl
		<< " " << nlohmann::json::meta()["name"] << " " << nlohmann::json::meta()["version"]["string"] << std::endl
		<< " OpenJPEG " << opj_version() << std::endl
		<< " NetCDF \"" << nc_inq_libvers() << "\"" << std::endl
		<< " GDAL " << GDALVersionInfo("RELEASE_NAME") << std::endl
		<< std::endl;

	if (argc < 2) {
		std::cerr << "Usage: " << CM_CONVERTER_NAME_STR
			<< " [-d S2_PATH] [-D CVAT_PATH] [-O OUT_PATH] [-r CVAT_XML -n NETCDF] [-b BANDS] [-R SUPERVISELY_DIR -t TILENAME -n NETCDF] [-A CVAT_SAI_PATH] [-S TILESIZE [-s SHRINK]] [-f DEFLATE_LEVEL] [-m RESAMPLING_METHOD] [-o OVERLAP] [--png] [--tiled] [-j JOBS] [-g EWKT] [--overwrite]" << std::endl
			<< "\twhere S2_PATH points to the .SAFE directory of an ESA S2 L2A or L1C product." << std::endl
			<< "\tCVAT_PATH points to the .CVAT directory (pre-processed ESA S2 product)." << std::endl
			<< "\tOUT_PATH points to the directory to store the output files (.CVAT directory, right next to the input .SAFE, by default)." << std::endl
			<< "\tCVAT_XML points to a CVAT annotations.xml file." << std::endl
			<< "\tCVAT_SAI_PATH points to the .CVAT directory with Segments.AI segmentation masks stored in subtiles." << std::endl
			<< "\tSUPERVISELY_DIR points to a directory with the Supervise.ly annotations files." << std::endl
			<< "\tNETCDF points to a NetCDF file to be updated with the rasterized annotations." << std::endl
			<< "\tBANDS is a comma-separated list of bands to process. If omitted, all bands are processed." << std::endl
			<< "\tTILENAME is the name of the tile to pick from the Supervise.ly directory." << std::endl
			<< "\tTILESIZE is the number of pixels per the edge of a square subtile (default: 512)." << std::endl
			<< "\tSHRINK is the factor by which to downscale from the 10 x 10 m^2 S2 bands (default: -1 (original size))." << std::endl
			<< "\tDEFLATE_LEVEL is the compression factor for NETCDF (between 0 and 9, where 9 is the highest level of compression)." << std::endl
			<< "\tRESAMPLING_METHOD defines a preferred way for resampling (point, box, cubic or sinc)." << std::endl
			<< "\tOVERLAP Overlap between sub-tiles (between 0 and 0.5)." << std::endl
			<< "\tJOBS Number of threads to parallelize to (0 for default, negative to use all available threads)." << std::endl
			<< "\tEWKT Geometry for area of interest (whole product, by default)." << std::endl
			<< "\t\tFor example: \"SRID=4326;Polygon ((22.64992375534184887 50.27513740160615185, 23.60228115218003708 50.35482161490517683, 23.54514084707420452 49.94024031630130622, 23.3153953947536472 50.21771699530808775, 22.64992375534184887 50.27513740160615185))\"" << std::endl;
		return 1;
	}

	GDALAllRegister();

	//! \note Magick relies on jasper for JP2 files, and jasper is not able to open the ESA S2 JP2 images.
	Magick::InitializeMagick(*argv);

	std::string arg_path_s2_dir, arg_path_cvat_dir, arg_path_rasterize, arg_path_nc, arg_path_cvat_sai_dir, arg_path_supervisely, arg_tilename;
	std::string arg_bands, arg_resampling_method, arg_path_out, arg_wkt_geom;
	unsigned int tilesize = 512;
	int downscale = -1;
	int deflatelevel = 9;
	float overlap = 0.0f;
	bool output_png = false;
	bool tiled_input = false;
	bool overwrite_subtiles = false;
	int num_jobs = 0;
	for (int i=0; i<argc; i++) {
		if (!strncmp(argv[i], "-d", 2))
			arg_path_s2_dir.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-O", 2))
			arg_path_out.assign(argv[i + 1]);
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
			tilesize = std::atoi(argv[i + 1]);
		else if (!strncmp(argv[i], "-s", 2))
			downscale = std::atoi(argv[i + 1]);
		else if (!strncmp(argv[i], "-t", 2))
			arg_tilename.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-f", 2))
			deflatelevel = std::atoi(argv[i + 1]);
		else if (!strncmp(argv[i], "-m", 2))
			arg_resampling_method.assign(argv[i + 1]);
		else if (!strncmp(argv[i], "-o", 2))
			overlap = std::atof(argv[i + 1]);
		else if (!strncmp(argv[i], "--png", 5))
			output_png = true;
		else if (!strncmp(argv[i], "--tiled", 7))
			tiled_input = true;
		else if (!strncmp(argv[i], "--overwrite", 11))
			overwrite_subtiles = true;
		else if (!strncmp(argv[i], "-j", 2))
			num_jobs = std::atoi(argv[i + 1]);
		else if (!strncmp(argv[i], "-g", 2))
			arg_wkt_geom.assign(argv[i + 1]);
	}

	if (arg_path_s2_dir.length() > 0) {
		ESA_S2_Image img;
		EmptyImageOperator img_op;

		std::filesystem::path path_dir_in(arg_path_s2_dir);
		if (!path_dir_in.is_absolute())
			path_dir_in = std::filesystem::absolute(arg_path_s2_dir);

		std::string str_path_dir_out;
		if (arg_path_out.empty())
			str_path_dir_out = path_dir_in.parent_path().string() + "/" + path_dir_in.stem().string() + ".CVAT";
		else
			str_path_dir_out = arg_path_out;
		std::filesystem::path path_dir_out(str_path_dir_out);

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
		img.set_deflate_factor(deflatelevel);
		img.set_overlap_factor(overlap);
		img.set_resampling_method(arg_resampling_method);
		img.set_png_output(output_png);
		img.set_tiled_input(tiled_input);
		img.set_num_threads(num_jobs);
		img.set_aoi_geometry(arg_wkt_geom);
		img.set_overwrite(overwrite_subtiles);

		img.process(path_dir_in, path_dir_out, img_op, bands);

	} else if (arg_path_cvat_dir.length() > 0) {
		std::cout << arg_path_cvat_dir << std::endl;

		std::cerr << "ERROR: Processing of pre-processed products is not supported yet." << std::endl;
		return 3;
	}

	if (arg_path_rasterize.length() > 0) {
		CVATRasterizer rasterizer;

		std::filesystem::path path_in(arg_path_rasterize);
		if (!path_in.is_absolute()) {
			path_in = std::filesystem::absolute(arg_path_rasterize);
		}
		std::filesystem::path path_out_nc(arg_path_nc);
		if (!path_out_nc.is_absolute()) {
			path_out_nc = std::filesystem::absolute(arg_path_nc);
		}

		rasterizer.image.set_deflate_level(deflatelevel);

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
		if (!path_in.is_absolute()) {
			path_in = std::filesystem::absolute(arg_path_supervisely);
		}
		std::filesystem::path path_out_nc(arg_path_nc);
		if (!path_out_nc.is_absolute()) {
			path_out_nc = std::filesystem::absolute(arg_path_nc);
		}

		r.image.set_deflate_level(deflatelevel);

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
		if (!path_in.is_absolute()) {
			path_in = std::filesystem::absolute(arg_path_cvat_sai_dir);
		}

		r.set_deflate_level(deflatelevel);

		if (std::filesystem::is_directory(path_in)) {
			r.convert(path_in);
		} else {
			std::cerr << "Directory " << path_in << " does not exist." << std::endl;
			return 1;
		}
	}

	return 0;
}

