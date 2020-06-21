// Generic raster image
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

#include <iostream>
#include <filesystem>
#include <Magick++.h>


class RasterImage {
	public:
		RasterImage();
		~RasterImage();

		virtual bool load(const std::filesystem::path &path) {}
		bool save(const std::filesystem::path &path);
		void clear();

		Magick::Image *subset;

		Magick::Geometry main_geometry;
		unsigned char main_depth;
		unsigned char main_num_components;

		bool scale(float f, bool point_filter);
		void remap_values(const unsigned char *values);
};

std::ostream &operator<<(std::ostream &out, const RasterImage &img);

