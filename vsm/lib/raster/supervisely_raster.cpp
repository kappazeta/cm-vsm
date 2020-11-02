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

#include "raster/supervisely_raster.hpp"


SuperviselyRaster::SuperviselyRaster() {}
SuperviselyRaster::~SuperviselyRaster() {}

bool SuperviselyRaster::load(const std::filesystem::path &path_dir_in, const std::string &product_tile_name) {
	if (subset != nullptr)
		clear();
    
    const std::string masks_dir = path_dir_in.string() + "/ds0/masks_machine";
    const std::string raster_filepath = masks_dir + "/" + product_tile_name + ".png";
    const std::string legend_filepath = path_dir_in.string() + "/obj_class_to_machine_color.json";
    
    if (std::filesystem::exists(raster_filepath)) {
        Magick::Image img(raster_filepath);
        img.quiet(false);
        img.type(Magick::GrayscaleType);
        
        main_geometry = img.size();
        main_depth = img.depth();
        main_num_components = 1;
	}

	return true;
}
