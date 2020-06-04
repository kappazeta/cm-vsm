cmake_minimum_required(VERSION 3.4.3)

set(VSM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include(${VSM_ROOT}/config.cmake OPTIONAL)
include(${VSM_ROOT}/cmake/modules/common.cmake)

# Make sure that the build files are in a separate directory
check_out_of_source()

# Initialize the project
project(CVAT_VSM)
set(VSM_VERSION "0.1")
vsm_common_init()

add_subdirectory(lib)
add_subdirectory(vsm)
