vsm_set_source_files(${CMAKE_CURRENT_SOURCE_DIR} VSM)

add_executable(cvat_vsm ${VSM_SRC} ${VSM_INC})
target_link_libraries(cvat_vsm libvsm openjp2 png expat)
set_target_properties(cvat_vsm PROPERTIES CXX_STANDARD 17)
