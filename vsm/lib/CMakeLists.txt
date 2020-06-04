set(LIBVSM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
vsm_set_source_files(${CMAKE_CURRENT_SOURCE_DIR} LIBVSM)

add_library(libvsm ${LIBVSM_SRC} ${LIBVSM_INC})
set_target_properties(libvsm PROPERTIES CXX_STANDARD 17)

