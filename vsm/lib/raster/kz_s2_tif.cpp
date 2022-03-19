// Processing of KappaZeta S2 raster products
//
// Copyright 2022 KappaZeta Ltd.
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

#include "raster/kz_s2_tif.hpp"
#include "raster/tif_image.hpp"
#include "raster/png_image.hpp"

#include "util/text.hpp"
#include <algorithm>
#include <math.h>
#include <set>

// GDAL
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>

std::vector<std::vector<unsigned char>> fill_poly_overlap(Polygon<int> &poly, float pixel_size_div);
std::vector<std::vector<unsigned char>> fill_whole(const AABB<int> &image_aabb, float tile_size_div);


const std::string KZ_S2_TIF_Image_Operator::data_type_name[KZ_S2_TIF_Image_Operator::DT_KZ_S2_COUNT] = {
	"B02", "B03", "B04", "B08", "B05", "B06", "B07", "B8A", "B11", "B12", "NDVI",
	"NDWI", "NDVIRE", "TCW", "TCV", "TCB", "YEL", "PSRI", "WRI", "Label"
};
const float KZ_S2_TIF_Image_Operator::scale_max[KZ_S2_TIF_Image_Operator::DT_KZ_S2_COUNT] = {
	0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

KZ_S2_TIF_Image::KZ_S2_TIF_Image():
	tile_size(512), f_downscale(1), f_overlap(0.0f), store_png(false),
	read_tiled(false), num_threads(0), geo_extracted(false) {
}
KZ_S2_TIF_Image::~KZ_S2_TIF_Image() {}

void KZ_S2_TIF_Image::set_tile_size(int tile_size) {
	this->tile_size = tile_size;
}

void KZ_S2_TIF_Image::set_downscale_factor(int f) {
	if (f <= 0)
		f_downscale = 1;
	else
		f_downscale = f;
}

void KZ_S2_TIF_Image::set_deflate_factor(int d) {
	deflate_factor = d;
}

void KZ_S2_TIF_Image::set_overlap_factor(float f) {
	if (f <= 0.0f)
		f_overlap = 0.0f;
	else if (f >= 0.5)
		f_overlap = 0.5f;
	else
		f_overlap = f;
}

void KZ_S2_TIF_Image::set_resampling_method(const std::string &m) {
	resampling_method_name = m;
}

void KZ_S2_TIF_Image::set_png_output(bool enabled) {
	store_png = enabled;
}

void KZ_S2_TIF_Image::set_aoi_geometry(const std::string &wkt_geom) {
	wkt_geom_aoi = wkt_geom;
}

std::string KZ_S2_TIF_Image::get_product_name_from_path(const std::filesystem::path &path) {
	for (auto it = path.begin(); it != path.end(); ++it) {
		if (endswith(*it, ".tif"))
			return *it;
	}
	return "";
}

std::string extract_index_date_kz(const std::filesystem::path &path) {
	// Path example: "/home/kappazeta/Documents/data/ams_data/1_036_20200504T094029_T34VEK.tif"
	std::string index_firstdate_result;
	std::string path_string = path.string(); 
	std::regex regexp("(?:\\d+)_(\\d+)_(\\d+T\\d+)_(T[\\dA-Z]+).*"); // Expression extracts ...ron_firstdate_index... from a full path file name
	std::smatch matches;
	
	std::regex_search(path_string, matches, regexp);
	if (matches.size() >= 3) {
		index_firstdate_result += matches.str(3); // Takes the second group (index)
		index_firstdate_result += "_";
		index_firstdate_result += matches.str(2); // Takes the first group (first date)
	}

	return index_firstdate_result;
}

//! \todo Create superclass to deduplicate code with ESA S2 rasters.

/**
 * @brief Extract the projection, geo-coordinates from the Sentinel-2 product. Produce the subtile mask for processing a subset of the product.
 * @param[in] path_in Reference to the axis-aligned bounding box of the whole product.
 * @param[in] image_aabb Reference to the axis-aligned bounding box of the whole product.
 * @param[in] tile_size_div Effective pixel size, accounting the overlap.
 */
void KZ_S2_TIF_Image::extract_geo(const std::filesystem::path &path_in, const AABB<int> &image_aabb, float tile_size_div) {
	GDALDataset *p_dataset = (GDALDataset *) GDALOpen(path_in.string().c_str(), GA_ReadOnly);
	if (p_dataset == NULL)
		throw RasterException(path_in, "Failed to load with GDAL");
	if (p_dataset->GetProjectionRef() != NULL) {
		std::cout << "Projection: " << p_dataset->GetProjectionRef() << std::endl;
	}

	// If there's an area of interest polygon, then fill the subtile mask with the polygon.
	subtile_mask.clear();
	if (wkt_geom_aoi.length() > 0) {
		std::cout << "Projecting AOI polygon into pixel coordinates." << std::endl;

		OGRGeometry *p_geom = nullptr;

		wkt_to_geom(wkt_geom_aoi, &p_geom);
		aoi_poly = proj_coords_to_raster<int>(p_geom, p_dataset);
		// Remove the last point, which is identical to the first.
		aoi_poly.remove(aoi_poly.size() - 1);
		// Increase the size of the polygon, to include overlap.
		aoi_poly.scale(1.0f + f_overlap);
		// Only keep the part of the polygon which is inside the raster.
		aoi_poly.clip_to_aabb(image_aabb);

		AABB<int> aabb = aoi_poly.get_aabb();
		aabb_buf = aabb.buffer(tile_size * f_overlap);
		aabb_buf.vmin.x /= image_aabb.vmax.x;
		aabb_buf.vmin.y /= image_aabb.vmax.y;
		aabb_buf.vmax.x /= image_aabb.vmax.x;
		aabb_buf.vmax.y /= image_aabb.vmax.y;

		//! \todo Use GDAL for reading raster data, too.

		if (aoi_poly.size() > 0) {
			subtile_mask = fill_poly_overlap(aoi_poly, tile_size_div);
		} else {
			GDALClose(p_dataset);
			throw RasterException(path_in, "No overlap between the area of interest polygon and raster");
		}
	// Otherwise take all the subtiles.
	} else {
		aabb_buf = AABB<float>(0, 0, 1, 1);
		subtile_mask = fill_whole(image_aabb, tile_size_div);
	}
	GDALClose(p_dataset);
}

// std::vector<std::string> split_str(std::string const &text, char delim) {
//! \todo Generalize the splitting logic, to deduplicate code.
bool KZ_S2_TIF_Image::process(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, KZ_S2_TIF_Image_Operator &op, std::vector<std::string> bands) {
	std::vector<unsigned int> band_ids;
	bool retval = true;

	// Build a vector of IDs of the bands to be processed.
	for (std::vector<std::string>::iterator it = bands.begin(); it != bands.end(); it++) {
		for (int i=0; i<KZ_S2_TIF_Image_Operator::DT_KZ_S2_COUNT; i++) {
			if (*it == KZ_S2_TIF_Image_Operator::data_type_name[i]) {
				band_ids.push_back(i);
			}
		}
	}

	retval = split_tiff(path_in, path_dir_out, op, band_ids);

	// Check if there's a mask file and split that, too.
	std::filesystem::path path_mask = path_in;
	path_mask.replace_filename(path_in.stem().string() + "_mask.tif");
	std::cout << path_mask << std::endl;

	if (std::filesystem::exists(path_mask)) {
		band_ids.clear();
		band_ids.push_back(KZ_S2_TIF_Image_Operator::DT_KZ_LABEL);
		split_tiff(path_mask, path_dir_out, op, band_ids);
	}

	return retval;
}

bool KZ_S2_TIF_Image::split_tiff(const std::filesystem::path &path_in, const std::filesystem::path &path_dir_out, KZ_S2_TIF_Image_Operator &op, std::vector<unsigned int> band_ids) {
	TIF_Image img_src;
	bool retval = true;

	float div_f = 1.0f;
	float tile_size_div = (tile_size - tile_size * f_overlap) / div_f;

	img_src.set_deflate_level(deflate_factor);
	img_src.set_num_threads(num_threads);

	// Propagate overlap factor for NetCDF metadata.
	img_src.f_overlap = f_overlap;
	// Assign product name from the input path.
	img_src.product_name = get_product_name_from_path(path_in);

	// Get image dimensions.
	retval &= img_src.load_header(path_in);

	std::cout << "Processing " << path_in << std::endl;

	// Extract image geo-coordinates, project area of interest polygon into pixel coordinates,
	// and produce a subtile mask, unless all of this has already been done.
	if (!geo_extracted) {
		std::cout << "Extracting geo-coordinates." << std::endl;
		AABB<int> image_aabb(img_src.main_geometry);
		extract_geo(path_in, image_aabb, tile_size_div);
		geo_extracted = true;
	}

	// NOTE:: Assume square tiles.
	int sx0, sy0, sx1, sy1, c;

	// Iterate over tiles in the output raster.
	Vector<int> p;
	std::vector<unsigned int>::iterator cit;
	for (p.x=0; p.x<(int)subtile_mask.size(); p.x++) {
		for (p.y=0; p.y<(int)subtile_mask[p.x].size(); p.y++) {
			if (subtile_mask[p.x][p.y] == 1) {
				// Coordinates in the source image (possibly with different dimensions).
				sx0 = aabb_buf.vmin.x * img_src.main_geometry.width() + floor(tile_size_div * p.x);
				sy0 = aabb_buf.vmin.y * img_src.main_geometry.height() + floor(tile_size_div * p.y);
				sx1 = ceil(sx0 + tile_size_div);
				sy1 = ceil(sy0 + tile_size_div);

				// It's possible that due to rounding errors, the tile would no longer be square.
				// For this case, we'll crop the additional row / column of pixels to square the tile once again.
				if (sx1 - sx0 > sy1 - sy0)
					sx1 = sx0 + sy1 - sy0;
				else if (sy1 - sy0 > sx1 - sx0)
					sy1 = sy0 + sx1 - sx0;

				// Account for overlap.
				sx1 += tile_size * f_overlap / div_f;
				sy1 += tile_size * f_overlap / div_f;

				// Process all the bands.
				for (cit=band_ids.begin(); cit!=band_ids.end(); cit++) {
					c = *cit;
					if (c == KZ_S2_TIF_Image_Operator::DT_KZ_LABEL)
						c = 0;

					// Load the source image.
					img_src.load_subset_channel(path_in, sx0, sy0, sx1, sy1, c);
					// "Normalize" the image.
					img_src.multiply(1.0f / KZ_S2_TIF_Image_Operator::scale_max[c]);

					if (img_src.subset->rows() != tile_size || img_src.subset->columns() != tile_size) {
						std::cout << "Invalid geometry " << img_src.subset->rows() << "x" << img_src.subset->columns() << " for subtile " << p.x << ", " << p.y << std::endl;
					}

					std::ostringstream ss_path_out, ss_path_out_png, ss_path_out_nc;
					ss_path_out << path_dir_out.string() << "/tile_" << p.x << "_" << p.y << "/";
					std::filesystem::create_directories(ss_path_out.str());

					// Add to NetCDF.
					ss_path_out_nc << ss_path_out.str() << extract_index_date_kz(path_in) << "_" << "tile" << "_" << p.x << "_" << p.y << ".nc";
					img_src.add_to_netcdf(ss_path_out_nc.str(), KZ_S2_TIF_Image_Operator::data_type_name[*cit]);

					// Save PNG.
					if (store_png) {
						ss_path_out_png << ss_path_out.str() << path_in.stem().string() << "_" << KZ_S2_TIF_Image_Operator::data_type_name[*cit];
						ss_path_out_png << "_" << "tile" << "_" << p.x << "_" << p.y << ".png";
						img_src.save(ss_path_out_png.str());
					}

					// Potential post-processing of the file.
					if (!op(ss_path_out.str()))
						return false;
				}
			}
		}
	}

	return retval;
}

