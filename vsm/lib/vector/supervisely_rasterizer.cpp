// Supervise.ly JSON vector layer rasterization to PNG and NetCDF.
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

#include "vector/supervisely_rasterizer.hpp"

#include <string.h>


const int SuperviselyPolygon::class_priority[SuperviselyPolygon::SVLY_COUNT] = {
	0, -4, -3, -2, -1
};

SuperviselyPolygon::SuperviselyPolygon(): label_index(0) {}

SuperviselyPolygon::~SuperviselyPolygon() {
	clear();
}

void SuperviselyPolygon::clear() {
	interior.clear();
	exterior.clear();
}

SuperviselyPolygon &SuperviselyPolygon::operator=(const SuperviselyPolygon &poly) {
	label_index = poly.label_index;
	interior = poly.interior;
	exterior = poly.exterior;

	return *this;
}

bool SuperviselyPolygon::operator<(const SuperviselyPolygon &poly) const {
	if (label_index < SVLY_COUNT && poly.label_index < SVLY_COUNT)
		return (class_priority[label_index] < class_priority[poly.label_index]);
	return false;
}

bool SuperviselyPolygon::parse_points(const nlohmann::json &j) {

	for (auto& p : j["exterior"])
		exterior.push_back(Magick::Coordinate((int) p[0], (int) p[1]));
	for (auto& p : j["interior"])
		interior.push_back(Magick::Coordinate((int) p[0], (int) p[1]));

	return true;
}

bool SuperviselyPolygon::set_label(const std::string &label) {
	label_index = 0;

	if (label == "CLEAR")
		label_index = SVLY_CLEAR;
	else if (label == "CLOUD")
		label_index = SVLY_CLOUD;
	else if (label == "SEMI_TRANSPARENT_CLOUD")
		label_index = SVLY_SEMI_TRANSPARENT_CLOUD;
	else if (label == "CLOUD_SHADOW")
		label_index = SVLY_CLOUD_SHADOW;
	return true;
}

SuperviselyRasterizer::SuperviselyRasterizer() {}

SuperviselyRasterizer::~SuperviselyRasterizer() {
	image.clear();
}

bool SuperviselyRasterizer::convert(const std::filesystem::path &path_dir_in, const std::string &product_tile_name, const std::filesystem::path &path_out_nc, const std::filesystem::path &path_out_png) {
	bool retval = true;

	const std::string vector_filepath = path_dir_in.string() + "/ds0/ann/" + product_tile_name + ".png.json";

	if (!std::filesystem::exists(vector_filepath)) {
		std::ostringstream stream;
		stream << "Vector file " << vector_filepath << " does not exist.";
		throw std::runtime_error(stream.str());
	}
	
	try {
		std::ifstream ifs(vector_filepath);
		nlohmann::json j = nlohmann::json::parse(ifs);

		// Set image geometry.
		image.main_geometry.width(j["size"]["width"]);
		image.main_geometry.height(j["size"]["height"]);

		// Iterate over polygons.
		for (auto& obj : j["objects"]) {
			if (obj["geometryType"] == "polygon") {
				SuperviselyPolygon p;

				p.set_label(obj["classTitle"]);
				p.parse_points(obj["points"]);

				polygons.push_back(p);
			}
		}

		rasterize();

		if (path_out_png.string().length() > 0)
			retval &= image.save(path_out_png);
		if (path_out_nc.string().length() > 0)
			retval &= image.add_to_netcdf(path_out_nc, "Label");

	} catch(std::exception &e) {
		retval = false;
		std::cerr << "ERROR: SuperviselyRasterizer: " << e.what() << std::endl;
	}

	return retval;
}

bool SuperviselyRasterizer::rasterize() {
	image.create_grayscale(image.main_geometry, 4, SuperviselyPolygon::SVLY_BACKGROUND);

	std::list<Magick::Drawable> drawlist;

	std::sort(polygons.begin(), polygons.end());

	double color;
	for (int i=0; i<polygons.size(); i++) {
		color = polygons[i].label_index / 255.0;

		if (polygons[i].exterior.size() > 2) {
			drawlist.push_back(Magick::DrawableStrokeAntialias(false));
			drawlist.push_back(Magick::DrawableStrokeColor(Magick::ColorGray(color)));
			drawlist.push_back(Magick::DrawableFillColor(Magick::ColorGray(color)));
			drawlist.push_back(Magick::DrawablePolygon(polygons[i].exterior));
		}
	}

	if (drawlist.size() > 0)
		image.subset->draw(drawlist);
	return true;
}


