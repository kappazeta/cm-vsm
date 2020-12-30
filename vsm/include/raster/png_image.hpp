// Tiled loading of a PNG image
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


class PNG_Image: public RasterImage {
	public:
		PNG_Image();
		~PNG_Image();

		bool load_header(const std::filesystem::path &path);

		bool load_subset(const std::filesystem::path &path, int da_x0, int da_y0, int da_x1, int da_y1);
};

