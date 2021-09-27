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
#include <Magick++.h>
// GDAL
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>


extern const float f_epsilon;

/**
 * An integer vector class
 */
template<class T>
class Vector {
public:
	/**
	 * Initialize a vector \f$(0, 0)\f$.
	 */
	Vector();

	/**
	 * Initialize a vector with the given coordinates.
	 */
	Vector(T x, T y);

	/**
	 * De-initialize the vector.
	 */
	~Vector();

	/**
	 * Add two vectors.
	 */
	Vector<T> operator+(const Vector<T> &a);
	/**
	 * Subtract two vectors.
	 */
	Vector<T> operator-(const Vector<T> &a);
	/**
	 * Scale a vector by a factor.
	 */
	Vector<T> operator*(float f);
	/**
	 * Scale a vector by a factor.
	 */
	Vector<T> operator/(float f);
	/**
	 * Assign the coordinates from another vector.
	 */
	Vector<T> &operator=(const Vector<T> &a);
	/**
	 * Add another vector to this one.
	 */
	Vector<T> &operator+=(const Vector<T> &a);
	/**
	 * Subtract another vector from this one.
	 */
	Vector<T> &operator-=(const Vector<T> &a);
	/**
	 * Scale this vector by a factor.
	 */
	Vector<T> &operator*=(float f);
	/**
	 * Scale this vector by a factor.
	 */
	Vector<T> &operator/=(float f);

	friend Vector operator-(const Vector &a, const Vector &b) {
		return Vector(a.x - b.x, a.y - b.y);
	}

	friend Vector operator+(const Vector &a, const Vector &b) {
		return Vector(a.x + b.x, a.y + b.y);
	}

	friend std::ostream &operator<<(std::ostream &os, const Vector &v) {
		os << "Vector(" << v.x << ", " << v.y << ")";
		return os;
	}

	/**
	 * @brief Convert a point in geocoordinates to pixel coordinates.
	 * @param[in] padfTransform GDAL geomatrix (GetGeoTransform()).
	 * @param[in] gx X-coordinate
	 * @param[in] gy Y-coordinate
	 * @return Reference to this vertex in pixel coordinates.
	 */
	Vector<T> &from_geo_coords(const double *padfTransform, double gx, double gy);

	T x, y;	///< Coordinates.
};

template<class T>
Vector<T> operator+(const Vector<T> &a, const Vector<T> &b);

template<class T>
Vector<T> operator-(const Vector<T> &a, const Vector<T> &b);

template<class T>
Vector<T> operator*(const Vector<T> &a, float f);

template<class T>
Vector<T> operator/(const Vector<T> &a, float f);

template<class T>
bool operator==(const Vector<T> &a, const Vector<T> &b);

template<class T>
class AABB {
public:
	AABB();
	AABB(const Vector<T> &vmin, const Vector<T> &vmax);
	AABB(T minx, T miny, T maxx, T maxy);
	AABB(const Magick::Geometry &geom);
	~AABB();

	Vector<T> vmin, vmax;

	Vector<T> &operator[](std::size_t idx);

	friend std::ostream &operator<<(std::ostream &os, const AABB &aabb) {
		os << "AABB(" << aabb.vmin << ", " << aabb.vmax << ")";
		return os;
	}

	/**
	 * @brief Buffer the axis-aligned bounding box.
	 * @param[in] buf_pixels Number of pixels to buffer outward (positive) or inward (negative).
	 * @return Buffered axis-aligned bounding box.
	 */
	AABB<T> buffer(float buf_pixels);

	bool contains(const Vector<T> &p) const;
	bool overlaps(const AABB<T> &aabb) const;

	size_t size();
};

template<class T>
class Polygon {
public:
	Polygon();
	Polygon(const std::vector<Vector<T>> &poly);
	~Polygon();

	std::vector<Vector<T>> v;

	Vector<T> &operator[](std::size_t idx);
	const Vector<T> &operator[](std::size_t idx) const;

	friend std::ostream &operator<<(std::ostream &os, const Polygon &poly) {
		os << "Polygon(";
		for (size_t i=0; i<poly.size(); i++) {
			os << poly[i];
			if (i < poly.size() - 1)
				os << ", ";
		}
		os << ")";
		return os;
	}

	/**
	 * @brief Calculate the axis-aligned bounding box of the polygon.
	 * @return Axis-aligned bounding box tightly surrounding the polygon.
	 */
	AABB<T> get_aabb() const;

	void clip_to_aabb(const AABB<T> &aabb);

	Vector<T> center() const;

	void scale(float f);

	/**
	 * @brief Checks if a point is in the polygon.
	 * @param[in] p Reference to the point to check.
	 * @return True if the point is in the polygon, otherwise false.
	 */
	bool contains(const Vector<T> &p) const;

	void push_back(const Vector<T> &p);
	bool remove(std::size_t idx);

	void clear();

	size_t size() const;
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
template<class T>
Polygon<T> proj_coords_to_raster(const OGRGeometry *p_geom, GDALDataset *p_dataset);

/**
 * @brief Test if two segments AB and CD intersect.
 * @param[in] pa Reference to point A.
 * @param[in] pb Reference to point B.
 * @param[in] pc Reference to point C.
 * @param[in] pd Reference to point D.
 * @param[out] pt Pointer to the intersection value along AB.
 * @param[out] p Pointer to the intersection point.
 * @return True if the line segments intersect, otherwise false.
 */
// bool line_in_line(const FVector &pa, const FVector &pb, const FVector &pc, const FVector &pd, float *pt, FVector *p);
