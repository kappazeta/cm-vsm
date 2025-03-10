//! @file
//! @brief Version macros and changelog
//
// Copyright 2021 - 2025 KappaZeta Ltd.
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

//! @brief Library / tool name.
#define CM_CONVERTER_NAME_STR		"cm-vsm"
//! @brief Library version.
#define CM_CONVERTER_VERSION_STR	"0.3.7"

/** \page Changelog
 * \par Changelog
 *
 * List of the `cm-vsm` library versions and the corresponding functional changes.
 *
 * Version | Changes
 * --------|--------
 * 0.3.7   | Support linear, hermite, hanning, hamming, blackman, gaussian, quadratic, catrom, mitchell, lanczos, bessel resampling methods.
 * 0.3.6   | Fix crash with empty geocoordinates string.
 * 0.3.5   | Fix crash without -T argument.
 * 0.3.4   | Fix CMake module for finding OpenJPEG v2.5.0.
 * 0.3.3   | Fix cget dependencies to json v3.11.2, OpenJPEG v2.5.0, NetCDF-c v4.9.0.
 * 0.3.2   | Limit processing to specific sub-tiles with '-T'.
 * 0.3.1   | Support for both MAJA flag formats. Use `-M`to specify.
 * 0.3.0   | Support splitting of KappaZeta rasters.
 * 0.2.13  | Skip subtiles with NetCDF which already has the band. Use `--overwrite` to override.
 * 0.2.12  | Fix an issue in area of interest geometry clipping.
 * 0.2.11  | Improved polyfill for tiny polygons. Error when no subtiles.
 * 0.2.10  | Minor improvements to error logging.
 * 0.2.9   | Fix issue for cropped S2 products. Default to sinc resampling unless specified. Display NetCDF version.
 * 0.2.7   | Improve error logging in conversion of Segments.AI annotations.
 * 0.2.6   | NetCDF layers no longer flipped.
 * 0.2.5   | CLI argument `-g` for AOI WKT geometry.
 * 0.2.4   | Support for IPL-UV DL-L8S2-UV rgbiswir classification mask. MAJA classification map update from THEIA MUSCATE format to native Sentinel-2 format.
 * 0.2.3   | Support for NASA GSFC labels.
 * 0.2.2   | Add missing `B07`.
 * 0.2.1   | Override output directory with the `-O` argument.
 * 0.1.21  | Speed optimizations.
 * 0.1.20  | CLI argument to disable PNG output.
 * 0.1.19  | Fix semi-transparent cloud class for Segments.AI labels.
 * 0.1.18  | Separate `NO_DATA`, `SATURATED_OR_DEFECTIVE` and `UNCLASSIFIED` from SCL classification scheme.
 * 0.1.17  | Support for CNES MAJA cloud mask.
 * 0.1.16  | Relative path support for command-line parameters.
 * 0.1.15  | Bands.nc filename was fixed for `-d` argument and unnecessary functions have been removed.
 * 0.1.14  | Fixed name for bands file from L2A rasters.
 * 0.1.13  | Shortened product name and tile indexes in NetCDF files' names.
 * 0.1.12  | Support for S2Cloudless with 10m, 20m or 60m resolution.
 * 0.1.11  | Configurable overlap between sub-tiles. Renamed from `cvat-vsm` to `cm-vsm`.
 * 0.1.10  | Configurable resampling method for ESA S2 rasters.
 * 0.1.9   | Configurable deflate level for label data conversion.
 * 0.1.8   | Configurable deflate level for NetCDF was added.
 * 0.1.7   | Store metadata (`version`, `product_name`, `last_modified`, `resampling_filter`) in NetCDF files.
 * 0.1.6   | Ensure square subtiles for 60m bands (need to re-generate NetCDF).
 * 0.1.5   | Fix dimensions for 60m bands (need to re-generate NetCDF).
 * 0.1.4   | Support for Francis & Mrziglod & Sidiropoulos classification masks.
 * 0.1.3   | Fmask, S2Cloudless subdirs now optional.
 * 0.1.2   | Support for Sinergise S2Cloudless masks.
 * 0.1.1   | First release with a version number.
 */
