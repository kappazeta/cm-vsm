# Checks build vs. source location. Prevents in-source builds.
macro(check_out_of_source)
	if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
		message(FATAL_ERROR "

CMake generation for this project is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   rm CMakeCache.txt
   cd build
   cmake -G \"Unix Makefiles\" ..
		")
	endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
endmacro(check_out_of_source)

# Common project initialization routines
macro(vsm_common_init)
	# Path to our CMake modules
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VSM_ROOT}/cmake/modules)

	# Output directories
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/lib)

	include_directories(${VSM_ROOT}/include)

	# Add generic flags to all builds,
	# Add debug flags to the debug build.
	add_compile_options(
		"-Wall" "-Wpedantic" "-Wextra" "-fexceptions"
		"$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
	)

	# Load cget modules.
	if(NOT DEFINED CGET_PREFIX)
		include(${VSM_ROOT}/build/cget/cget/cget.cmake)
	endif(NOT DEFINED CGET_PREFIX)

	# Default to an install prefix of /usr.
	IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		SET(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Default prefix: /usr" FORCE)
	ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endmacro(vsm_common_init)

# A macro to search for source files
macro(vsm_set_source_files src_path name)
	file(GLOB ${name}_SRC ${src_path}/*.cpp ${src_path}/*/*.cpp)
	file(GLOB ${name}_INC ${VSM_ROOT}/include/${name}/*.hpp ${VSM_ROOT}/include/${name}/*/*.hpp)
	include_directories(${root_path}/include)
endmacro(vsm_set_source_files)

# Create a "doc" target for documentation
macro(vsm_doc_target)
	find_package(Doxygen)
	if(NOT DOXYGEN_FOUND)
		message(WARNING "Doxygen is needed to build the documentation.")
	else(NOT DOXYGEN_FOUND)
		set(logo_in ${VSM_ROOT}/doc/kz_logo.png)
		set(logo_out ${CMAKE_CURRENT_BINARY_DIR}/kz_logo.png)
		set(doxyfile_in ${VSM_ROOT}/doc/cm_vsm.doxyfile)
		set(doxyfile_out ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)

		configure_file(${doxyfile_in} ${doxyfile_out} @ONLY)
		configure_file(${logo_in} ${logo_out} COPYONLY)

		add_custom_target(
			doc
			COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile_out}
			WORKING_DIRECTORY ${VSM_ROOT}/doc
			COMMENT "Generating API documentation with Doxygen"
			VERBATIM
		)
	endif()
endmacro(vsm_doc_target)
