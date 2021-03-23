// Segments.AI annotations format
//
// Copyright 2021 KappaZeta Ltd.
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

#include "raster/segmentsai_raster.hpp"
#include "vector/cvat_rasterizer.hpp"
#include "util/text.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

SegmentsAIRaster::SegmentsAIRaster() {}
SegmentsAIRaster::~SegmentsAIRaster() {}

bool SegmentsAIRaster::load(const std::filesystem::path &mask_path, const std::filesystem::path &classes_path) {
	std::map<unsigned short, float> class_map;

	if (subset != nullptr)
		clear();

	// Load the RGB raster.
	Magick::Image img(mask_path.string());
	img.quiet(false);

	// Load metadata.
	PixelRGB8 c_cloud, c_cloud_shadow, c_semi_cloud, c_clear, c_undefined;
	std::ifstream ifs(classes_path.string());
	nlohmann::json j = nlohmann::json::parse(ifs);

	// Check file format version.
	if (j["format_version"] != "0.0.1") {
		std::ostringstream stream;
		stream << "Classes file " << classes_path.string() << " has unsupported version " << j["format_version"];
		throw std::runtime_error(stream.str());
	}

	// Map the label IDs to our own class colors.
	float col;
	for (const auto &entry: j["label_map"]) {
		if (entry["category_name"] == "cloud")
			col = CVATPolygon::CV_CLOUD / 255.0f;
		else if (entry["category_name"] == "cloud_shadow")
			col = CVATPolygon::CV_CLOUD_SHADOW / 255.0f;
		else if (entry["category_name"] == "clear")
			col = CVATPolygon::CV_CLEAR / 255.0f;
		else if (entry["category_name"] == "semi_transparent_cloud")
			col = CVATPolygon::CV_CLEAR / 255.0f;
		else if (entry["category_name"] == "not_defined")
			col = CVATPolygon::CV_UNDEFINED / 255.0f;
		else if (entry["category_name"] == "invalid")
			col = CVATPolygon::CV_INVALID / 255.0f;

		class_map.insert({(unsigned short) entry["id"], col});
	}

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
		// Pixel value 0 is always mapped to the background color.
		if (pixel.r > 0) {
			col = class_map.at(pixel.r);
			dpx[i] = Magick::ColorGray(col);
		}
	}

	subset->syncPixels();

	return true;
}

bool SegmentsAIRaster::convert(const std::filesystem::path &path_dir) {
	bool retval = true;
	std::string path_mask = "", path_classes = "", path_nc = "";

	// Look for tiles with SegmentsAI classificatgion mask files.
	for (const auto &tile_entry: std::filesystem::directory_iterator(path_dir.string())) {
		path_mask = tile_entry.path().string() + "/segments_ai_classification_mask.png";
		path_classes = tile_entry.path().string() + "/segments_ai_classes.json";
		path_nc = tile_entry.path().string() + "/" + extract_index_firstdate(path_dir.stem()) + "_" + extract_tile_id(tile_entry.path().string()) + ".nc";

		if (std::filesystem::exists(path_mask) && std::filesystem::exists(path_classes)) {
			retval &= load(path_mask, path_classes);
			if (retval)
				retval &= save(path_mask + "_converted.png");
			if (retval)
				retval &= add_to_netcdf(path_nc, "Label");
			std::cout << tile_entry << std::endl;
		}
	}

	return retval;
}
