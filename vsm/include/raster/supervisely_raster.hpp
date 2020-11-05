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

#pragma once

#include "raster/raster_image.hpp"

#include <filesystem>


class SuperviselyRaster: public RasterImage {
	public:
		SuperviselyRaster();
		~SuperviselyRaster();

	void set_tile_name(const std::string &product_tile_name);

	bool load(const std::filesystem::path &path, const std::string &product_tile_name);

	bool convert(const std::filesystem::path &path_dir, const std::filesystem::path &path_nc);

	protected:
		std::string tile_name;
};
