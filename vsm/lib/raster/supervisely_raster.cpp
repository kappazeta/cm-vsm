// Supervisely annotations format
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

#include "raster/netcdf_interface.hpp"
#include "raster/supervisely_raster.hpp"
#include "vector/cvat_rasterizer.hpp"
#include "util/text.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

SuperviselyRaster::SuperviselyRaster() {}
SuperviselyRaster::~SuperviselyRaster() {}

bool SuperviselyRaster::load(const std::filesystem::path &path_dir_in, const std::string &product_tile_name) {
	if (subset != nullptr)
		clear();

	const std::string masks_dir = path_dir_in.string() + "/ds0/masks_machine";
	const std::string raster_filepath = masks_dir + "/" + product_tile_name + ".png";
	const std::string legend_filepath = path_dir_in.string() + "/obj_class_to_machine_color.json";

	if (!std::filesystem::exists(raster_filepath)) {
		std::ostringstream stream;
		stream << "Raster file " << raster_filepath << " does not exist.";
		throw std::runtime_error(stream.str());
	}
	if (!std::filesystem::exists(legend_filepath)) {
		std::ostringstream stream;
		stream << "Legend file " << legend_filepath << " does not exist.";
		throw std::runtime_error(stream.str());
	}

	// Load the RGB raster.
	Magick::Image img(raster_filepath);
	img.quiet(false);

	// Load metadata.
	PixelRGB8 c_cloud, c_cloud_shadow, c_semi_cloud, c_clear, c_undefined;
	std::ifstream ifs(legend_filepath);
	nlohmann::json j = nlohmann::json::parse(ifs);
	// Pixel colors for different classes.
	c_cloud.set(j["CLOUD"]);
	c_semi_cloud.set(j["SEMI_TRANSPARENT_CLOUD"]);
	c_cloud_shadow.set(j["CLOUD_SHADOW"]);
	c_clear.set(j["CLEAR"]);
	c_undefined.set(j["UNDEFINED"]);

	// Create output raster (grayscale).
	subset = create_grayscale(img.size(), 8, CVATPolygon::CV_BACKGROUND);

	unsigned int w = img.columns();
	unsigned int h = img.rows();
	unsigned int size = w * h;

	Magick::PixelPacket *spx = img.getPixels(0, 0, w, h);
	Magick::PixelPacket *dpx = subset->getPixels(0, 0, w, h);
	PixelRGB8 pixel;

	for (unsigned int i=0; i<size; i++) {
		pixel = PixelRGB8(spx[i]);
		if (pixel == c_cloud)
			dpx[i] = Magick::ColorGray(CVATPolygon::CV_CLOUD / 255.0f);
		else if (pixel == c_cloud_shadow)
			dpx[i] = Magick::ColorGray(CVATPolygon::CV_CLOUD_SHADOW / 255.0f);
		else if (pixel == c_semi_cloud)
			dpx[i] = Magick::ColorGray(CVATPolygon::CV_SEMI_TRANSPARENT_CLOUD / 255.0f);
		else if (pixel == c_clear)
			dpx[i] = Magick::ColorGray(CVATPolygon::CV_CLEAR / 255.0f);
		else if (pixel == c_undefined)
			dpx[i] = Magick::ColorGray(CVATPolygon::CV_CLEAR / 255.0f);
	}

	subset->syncPixels();

	return true;
}

bool SuperviselyRaster::convert(const std::filesystem::path &path_dir, const std::string &tile_name, const std::filesystem::path &path_nc) {
	NetCDFInterface nci;
	bool retval = true;

	std::filesystem::path path_out_png(path_dir.parent_path().string() + "/supervisely_raster_" + tile_name + ".png");

	retval &= load(path_dir, tile_name);

	if (retval)
		retval &= save(path_out_png);
	if (retval)
		retval &= nci.add_to_file(path_nc, "Label", *this);

	return retval;
}

