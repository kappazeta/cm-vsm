//! @file
//! @brief Geometry classes for vector layers
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

#include <sstream>
#include <iostream>
// GDAL
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>


//! \todo Math operators (need at least scaling)

/**
 * A floating-point vertex class
 */
class FVertex {
public:
	/**
	 * Initialize a vertex \f$(0, 0)\f$.
	 */
	FVertex();

	/**
	 * Initialize a vertex with the given coordinates.
	 */
	FVertex(float x, float y);

	/**
	 * De-initialize the vertex.
	 */
	~FVertex();

	float x, y;	///< Coordinates.
};


/**
 * An integer vertex class
 */
class IVertex {
public:
	/**
	 * Initialize a vertex \f$(0, 0)\f$.
	 */
	IVertex();

	/**
	 * Initialize a vertex with the given coordinates.
	 */
	IVertex(int x, int y);

	/**
	 * De-initialize the vertex.
	 */
	~IVertex();

	int x, y;	///< Coordinates.
};

/**
 * @brief Class for exceptions related to GDAL OGR operations.
 */
class GDALOGRException: public std::exception {
	public:
		/**
		 * @param[in] msg Reference to the error message.
		 * @param[in] retval Error code from GDAL OGR operations.
		 */
		GDALOGRException(const std::string &msg, OGRErr retval) {
			message = msg;
			ogr_retval = retval;

			std::ostringstream ss;
			ss << "GDAL OGR error : " << message << ", ";
			switch(retval) {
				case OGRERR_NONE: ss << "No error"; break;
				case OGRERR_NOT_ENOUGH_DATA: ss << "Not enough data"; break;
				case OGRERR_NOT_ENOUGH_MEMORY: ss << "Not enough memory"; break;
				case OGRERR_UNSUPPORTED_GEOMETRY_TYPE: ss << "Unsupported geometry type"; break;
				case OGRERR_UNSUPPORTED_OPERATION: ss << "Unsupported operation"; break;
				case OGRERR_CORRUPT_DATA: ss << "Corrupt data"; break;
				default:
				case OGRERR_FAILURE: ss << "Generic failure"; break;
				case OGRERR_UNSUPPORTED_SRS : ss << "Unsupported SRS"; break;
				case OGRERR_INVALID_HANDLE: ss << "Invalid handle"; break;
				case OGRERR_NON_EXISTING_FEATURE: ss << "Non-existing feature"; break;
			}
			full_message.assign(ss.str());
		}

		std::string message;	///< Error message.
		int ogr_retval;	///< Error code from GDAL OGR operations.

		/**
		 * Pointer to the full message C string.
		 */
		virtual const char* what() const throw() {
			return full_message.c_str();
		}

	protected:
		std::string full_message;	///< Full human-readable message.
};

/**
 * @brief Class for exceptions related to GDAL CPL operations.
 */
class GDALCPLException: public std::exception {
	public:
		/**
		 * @param[in] msg Reference to the error message.
		 * @param[in] retval Error code from GDAL OGR operations.
		 */
		GDALCPLException(const std::string &msg) {
			message = msg;

			cpl_retval = CPLGetLastErrorType();
			cpl_errnum = CPLGetLastErrorNo();

			std::ostringstream ss;
			ss << "GDAL CPL error : " << message << ", " << CPLGetLastErrorMsg();
			full_message.assign(ss.str());
		}

		std::string message;	///< Error message.
		CPLErr cpl_retval;	///< Error code from the last GDAL CPL operation.
		CPLErrorNum cpl_errnum;	///< Error number from the last GDAL CPL operation;

		/**
		 * Pointer to the full message C string.
		 */
		virtual const char* what() const throw() {
			return full_message.c_str();
		}

	protected:
		std::string full_message;	///< Full human-readable message.
};

/**
 * @brief Convert a point in geocoordinates to pixel coordinates.
 * @param[in] padfTransform GDAL geomatrix (GetGeoTransform()).
 * @param[in] x X-coordinate
 * @param[in] y Y-coordinate
 * @return Vertex in pixel coordinates.
 */
IVertex geo_to_pixel_coords(const double *padfTransform, double x, double y);

/**
 * @brief Converts a WKT string with SRID into GDAL OGR geometry.
 * @param[in] wkt Reference to the WKT string, in the following format: `SRID=4326;Polygon ((22.64992375534184887 50.27513740160615185, 23.60228115218003708 50.35482161490517683, 23.54514084707420452 49.94024031630130622, 23.3153953947536472 50.21771699530808775, 22.64992375534184887 50.27513740160615185))`.
 * @param[out] p_geom Pointer to the address where to store the pointer to the output OGR geometry.
 * @return Pointer to the output OGR geometry.
 * @throw GDALOGRException
 */
OGRGeometry *wkt_to_geom(const std::string &wkt, OGRGeometry **p_geom);

/**
 * @brief Project the geometry into pixel coordinates on the raster dataset.
 * @param[in] p_geom Pointer to the geometry to project.
 * @param[in] p_dataset Pointer to the dataset to project to.
 * @return List of vertices in pixel coordinates.
 * @throw GDALCPLException, GDALOGRException
 */
std::vector<IVertex> proj_coords_to_raster(const OGRGeometry *p_geom, GDALDataset *p_dataset);

