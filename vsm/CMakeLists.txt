cmake_minimum_required(VERSION 3.4.3)

set(VSM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include(${VSM_ROOT}/config.cmake OPTIONAL)
include(${VSM_ROOT}/cmake/modules/common.cmake)

# Make sure that the build files are in a separate directory
check_out_of_source()

# Initialize the project
project(CVAT_VSM)
set(VSM_VERSION "0.2")
vsm_common_init()

find_package(OpenJPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(GDAL REQUIRED)
find_package(EXPAT REQUIRED)
include_directories(${OPENJPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS})

add_subdirectory(lib)
add_subdirectory(vsm)
