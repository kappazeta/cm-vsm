# ESA S2 product converter for cloud mask labeling and processing
#
# Copyright 2021 KappaZeta Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.4.3)

set(VSM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include(${VSM_ROOT}/config.cmake OPTIONAL)
include(${VSM_ROOT}/cmake/modules/common.cmake)

# Make sure that the build files are in a separate directory
# check_out_of_source()

# Initialize the project
project(CM_VSM)
set(VSM_VERSION "0.3")
vsm_common_init()

find_package(OpenJPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(GDAL REQUIRED)
find_package(EXPAT REQUIRED)
find_package(GraphicsMagick REQUIRED)
find_package(NetCDF REQUIRED)
include_directories(${OPENJPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS} ${MAGICK_INCLUDE_DIR} ${NETCDF_INCLUDES})

add_subdirectory(lib)
add_subdirectory(vsm)

vsm_doc_target()
