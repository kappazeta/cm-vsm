find_package(OpenJPEG REQUIRED)
include_directories(${OPENJPEG_INCLUDE_DIR})

vsm_set_source_files(${CMAKE_CURRENT_SOURCE_DIR} VSM)

add_executable(cvat_vsm ${VSM_SRC} ${VSM_INC})
target_link_libraries(cvat_vsm openjp2)
