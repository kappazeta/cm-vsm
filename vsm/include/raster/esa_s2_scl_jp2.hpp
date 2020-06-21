// ESA S2 Scene Classification Map, in JP2 format
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

#include "raster/jp2_image.hpp"

#include <filesystem>


class ESA_S2_SCL_JP2_Image: public JP2_Image {
	public:
		ESA_S2_SCL_JP2_Image();
		~ESA_S2_SCL_JP2_Image();
		
		static const std::string class_names[12];
		unsigned char class_map[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
};

