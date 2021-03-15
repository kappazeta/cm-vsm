// Version macros
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

#pragma once

#define CM_CONVERTER_NAME_STR		"cvat-vsm"
#define CM_CONVERTER_VERSION_STR	"0.1.10"

// Changelog
//  0.1.10  Configurable resampling method for ESA S2 rasters.
//  0.1.9   Configurable deflate level for label data conversion.
//  0.1.8   Configurable deflate level for NetCDF was added.
//  0.1.7   Store metadata (version, product_name, last_modified, resampling_filter) in NetCDF files.
//  0.1.6   Ensure square subtiles for 60m bands (need to re-generate NetCDF).
//  0.1.5   Fix dimensions for 60m bands (need to re-generate NetCDF).
//  0.1.4   Support for Francis & Mrziglod & Sidiropoulos classification masks.
//  0.1.3   Fmask, S2Cloudless subdirs now optional.
//  0.1.2   Support for Sinergise S2Cloudless masks.
//  0.1.1   First release with a version number.
