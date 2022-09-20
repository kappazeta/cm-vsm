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


extern const float f_epsilon;	///< A very small value for floating-point comparisons.

/**
 * @brief A geometric vector class
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
	 * Initialize with a vector from a different base type.
	 */
	template<class U>
	Vector(const Vector<U> &a) {
		x = a.x;
		y = a.y;
	}

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
	 * Assign coordinates from a vector with a different base type.
	 */
	template<class U>
	Vector<T> &operator=(const Vector<U> &a) {
		x = a.x;
		y = a.y;
		return *this;
	}

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

	/**
	 * Subtract one vector from another.
	 */
	friend Vector operator-(const Vector &a, const Vector &b) {
		return Vector(a.x - b.x, a.y - b.y);
	}

	/**
	 * Add one vector to another.
	 */
	friend Vector operator+(const Vector &a, const Vector &b) {
		return Vector(a.x + b.x, a.y + b.y);
	}

	/**
	 * Describe the vector for an output stream.
	 */
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

	/**
	 * @Brief Vector magnitude or length.
	 */
	double length() const;

	T x, y;	///< Coordinates.
};

/**
 * Add two vectors of same base type.
 */
template<class T>
Vector<T> operator+(const Vector<T> &a, const Vector<T> &b);

/**
 * Subtract two vectors of same base type.
 */
template<class T>
Vector<T> operator-(const Vector<T> &a, const Vector<T> &b);

/**
 * Scale a vector by a factor.
 */
template<class T>
Vector<T> operator*(const Vector<T> &a, float f);

/**
 * Scale a vector by a factor.
 */
template<class T>
Vector<T> operator/(const Vector<T> &a, float f);

/**
 * Compare two vectors of the same base type.
 */
template<class T>
bool operator==(const Vector<T> &a, const Vector<T> &b);

/**
 * "Cross product" between two 2D vectors.
 * https://stackoverflow.com/a/243984/1692112
 */
template<class T>
T cross_product(const Vector<T> &a, const Vector<T> &b);


/**
 * @brief Axis-aligned bounding box.
 */
template<class T>
class AABB {
public:
	/**
	 * Initialize a degenerate bounding box at (0, 0).
	 */
	AABB();

	/**
	 * Initialize a bounding box between vectors of minimum and maximum coordinates.
	 * @param[in] vmin Reference to a vector of minimum coordinates (x_min, y_min).
	 * @param[in] vmax Reference to a vector of maximum coordinates (x_max, y_max).
	 */
	AABB(const Vector<T> &vmin, const Vector<T> &vmax);

	/**
	 * Initialize a bounding box with specified coordinates.
	 * @param[in] minx Minimum X-coordinate.
	 * @param[in] miny Minimum Y-coordinate.
	 * @param[in] maxx Maximum X-coordinate.
	 * @param[in] maxy Maximum Y-coordinate.
	 */
	AABB(T minx, T miny, T maxx, T maxy);

	/**
	 * Initialize a bounding box from a GraphicsMagick geometry.
	 */
	AABB(const Magick::Geometry &geom);

	/**
	 * Deinitialize the bounding box.
	 */
	~AABB();

	/**
	 * Initialize the bounding box from another one with a different base type.
	 */
	template<class U>
	AABB(const AABB<U> &a) {
		vmin = a.vmin;
		vmax = a.vmax;
	}

	Vector<T> vmin;	///< Minimum coordinates (x_min, y_min).
	Vector<T> vmax;	///< Maximum coordinates (x_max, y_max).

	/**
	 * Index the bounding box as a list of two vectors.
	 * @param[in] idx Index, [0, 2).
	 */
	Vector<T> &operator[](std::size_t idx);

	/**
	 * Assign coordinates from another bounding box with a different base type.
	 */
	template<class U>
	AABB<T>& operator=(const AABB<U> &a) {
		vmin = a.vmin;
		vmax = a.vmax;
		return *this;
	}

	/**
	 * Describe the bounding box for an output stream.
	 */
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

	/**
	 * @brief Check if the bounding box contains a point.
	 * @param[in] p Reference to the point to check against the bounding box.
	 * @return True if the point is inside or on the edge of the bounding box. False otherwise.
	 */
	bool contains(const Vector<T> &p) const;

	/**
	 * @brief Check if the bounding box overlaps another.
	 * @param[in] aabb Reference to the other bounding box to check against.
	 * @return True if the bounding boxes are inside each-other, overlap, or touch one another. False otherwise.
	 */
	bool overlaps(const AABB<T> &aabb) const;

	/**
	 * @brief Number of vectors in the bounding box.
	 * @return Always 2.
	 */
	size_t size() const;
};

/**
 * @brief Simple geometry with an arbitrary number of corners but no cutouts.
 */
template<class T>
class Polygon {
public:
	/**
	 * Initialize the polygon.
	 */
	Polygon();

	/**
	 * Initialize the polygon from a list of vectors of the same base type.
	 */
	Polygon(const std::vector<Vector<T>> &poly);

	/**
	 * Deinitialize the polygon.
	 */
	~Polygon();

	std::vector<Vector<T>> v;	///< List of polygon vertices.

	/**
	 * Index a vertex in the polygon.
	 * @param[in] idx Index of the vertex to request.
	 * @throw std::out_of_range
	 */
	Vector<T> &operator[](std::size_t idx);

	/**
	 * Index a vertex in the polygon (read-only).
	 * @param[in] idx Index of the vertex to request.
	 * @throw std::out_of_range
	 */
	const Vector<T> &operator[](std::size_t idx) const;

	/**
	 * Describe the polygon for an output stream.
	 */
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

	/**
	 * @brief Clip the polygon vertices to a bounding box.
	 * @param[in] aabb Reference to the bounding box to cut the polygon to.
	 */
	void clip_to_aabb(const AABB<T> &aabb);

	/**
	 * @brief Calculate the central point (mean coordinates) of the polygon.
	 */
	Vector<T> center() const;

	/**
	 * @brief Scale the polygon by a factor, around its center.
	 * @param[in] Scaling factor, 1.0f to leave the polygon unchanged.
	 */
	void scale(float f);

	/**
	 * @brief Checks if a point is in the polygon.
	 * @param[in] p Reference to the point to check.
	 * @return True if the point is in the polygon, otherwise false.
	 */
	bool contains(const Vector<T> &p) const;

	/**
	 * @brief Add a vertex after the last point in the polygon.
	 * @param[in] p Reference to the point to add.
	 */
	void push_back(const Vector<T> &p);

	/**
	 * @brief Remove a vertex at the specified index.
	 * @param[in] idx Index of the point to remove.
	 * @return True if the point was removed, otherwise false.
	 */
	bool remove(std::size_t idx);

	/**
	 * @brief Remove all vertices of the polygon.
	 */
	void clear();

	/**
	 * @brief Count polygon vertices.
	 */
	size_t size() const;

	double area() const;
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
		 * @brief Pointer to the full message C string.
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
		 * @brief Pointer to the full message C string.
		 */
		virtual const char* what() const throw() {
			return full_message.c_str();
		}

	protected:
		std::string full_message;	///< Full human-readable message.
};

/**
 * @brief Extract coordinate pairs from a comma-separated string.
 * @param[in] text Reference to the input text.
 * @param delim Single character delimiter to use for splitting the text.
 * @return An std::vector of integer coordinates.
 */
std::vector<Vector<int>> extract_coords(std::string const &text, char delim, char coord_delim);

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
 * @brief Polygon-fill the subtile mask. Each pixel of the subtile mask corresponds to a sub-tile of the whole product.
 * @param[in] poly Reference to the polygon to fill onto the mask.
 * @param[in] pixel_size_div Effective pixel size, accounting the overlap.
 * @return 2D array of values (0 - skipped, 1 - sub-tile within area of interest).
 */
std::vector<std::vector<unsigned char>> fill_poly_overlap(Polygon<int> &poly, float pixel_size_div);

/**
 * @brief Polygon-fill the subtile mask. Each pixel of the subtile mask corresponds to a sub-tile of the whole product.
 * @param[in] image_aabb Reference to the axis-aligned bounding box of the whole product.
 * @param[in] poly Reference to the polygon to fill onto the mask.
 * @param[in] pixel_size_div Effective pixel size, accounting the overlap.
 * @param[in] buffer_out Buffer by filling surrounding pixels.
 * @return 2D array of values (0 - skipped, 1 - sub-tile within area of interest).
 */
std::vector<std::vector<unsigned char>> fill_poly_overlap(const AABB<int> &image_aabb, Polygon<int> &poly, float pixel_size_div, bool buffer_out);

/**
 * @brief Fill the entire subtile mask. Each pixel of the subtile mask corresponds to a sub-tile of the whole product.
 * @param[in] image_aabb Reference to the axis-aligned bounding box of the whole product.
 * @param[in] tile_size_div Effective pixel size, accounting the overlap.
 * @param[in] value Value to fill with.
 * @return 2D array of values 1 - all sub-tiles within the area of interest.
 */
std::vector<std::vector<unsigned char>> fill_whole(const AABB<int> &image_aabb, float tile_size_div, unsigned char value);

/**
 * @brief Apply a mask over another mask (multiply them), to obtain an intersection.
 * @param[in] mask_in Reference to the input mask.
 * @param[in] mask_to_apply Reference to the mask to apply on the input mask.
 * @return Intersection of the two masks.
 */
std::vector<std::vector<unsigned char>> apply_mask(const std::vector<std::vector<unsigned char>> &mask_in, const std::vector<std::vector<unsigned char>> &mask_to_apply);
