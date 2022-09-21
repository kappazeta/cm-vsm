// Geometry classes for vector layers
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

#include "util/geometry.hpp"
#include "util/text.hpp"
#include <cstring>
// GDAL
#include <ogrsf_frmts.h>
#include <ogr_spatialref.h>


const float f_epsilon = 1e-5f;

template<class T>
Vector<T>::Vector(): x(0), y(0) {}

template<class T>
Vector<T>::Vector(T x, T y): x(x), y(y) {}

template<class T>
Vector<T>::~Vector() {}

template<class T>
Vector<T> Vector<T>::operator-(const Vector<T> &a) {
	return Vector<T>(x - a.x, y - a.y);
}

template<class T>
Vector<T> Vector<T>::operator+(const Vector<T> &a) {
	return Vector<T>(x + a.x, y + a.y);
}

template<>
Vector<int> Vector<int>::operator*(float f) {
	return Vector<int>((int) round(x * f), (int) round(y * f));
}

template<>
Vector<float> Vector<float>::operator*(float f) {
	return Vector<float>(x * f, y * f);
}

template<>
Vector<int> Vector<int>::operator/(float f) {
	return Vector<int>((int) round(x / f), (int) round(y / f));
}

template<>
Vector<float> Vector<float>::operator/(float f) {
	return Vector<float>(x / f, y / f);
}

template<>
bool operator==(const Vector<int> &a, const Vector<int> &b) {
	return (a.x == b.x && a.y == b.y);
}

template<>
bool operator==(const Vector<float> &a, const Vector<float> &b) {
	return (abs(a.x - b.x) < f_epsilon && abs(a.y - b.y) < f_epsilon);
}

template<class T>
Vector<T> &Vector<T>::operator=(const Vector<T> &a) {
	this->x = a.x;
	this->y = a.y;
	return *this;
}

template<class T>
Vector<T> &Vector<T>::operator-=(const Vector<T> &a) {
	this->x -= a.x;
	this->y -= a.y;
	return *this;
}

template<class T>
Vector<T> &Vector<T>::operator+=(const Vector<T> &a) {
	this->x += a.x;
	this->y += a.y;
	return *this;
}

template<>
Vector<int> &Vector<int>::operator*=(float f) {
	this->x = (int) round(this->x * f);
	this->y = (int) round(this->y * f);
	return *this;
}

template<>
Vector<float> &Vector<float>::operator*=(float f) {
	this->x *= f;
	this->y *= f;
	return *this;
}

template<>
Vector<int> &Vector<int>::operator/=(float f) {
	this->x = (int) round(this->x / f);
	this->y = (int) round(this->y / f);
	return *this;
}

template<>
Vector<float> &Vector<float>::operator/=(float f) {
	this->x /= f;
	this->y /= f;
	return *this;
}

template<>
Vector<int> &Vector<int>::from_geo_coords(const double *padfTransform, double gx, double gy) {
	// https://stackoverflow.com/a/58814574/1692112
	this->x = (int) ((gx - padfTransform[0]) / padfTransform[1]);
	this->y = (int) ((gy - padfTransform[3]) / padfTransform[5]);

	return *this;
}

template<>
Vector<float> &Vector<float>::from_geo_coords(const double *padfTransform, double gx, double gy) {
	// https://stackoverflow.com/a/58814574/1692112
	this->x = (gx - padfTransform[0]) / padfTransform[1];
	this->y = (gy - padfTransform[3]) / padfTransform[5];

	return *this;
}

template<class T>
double Vector<T>::length() const {
	return sqrt(x*x + y*y);
}

template<class T>
T cross_product(const Vector<T> &a, const Vector<T> &b) {
	return a.x*b.y - a.y*b.x;
}


template<class T>
AABB<T>::AABB() {}

template<class T>
AABB<T>::AABB(const Vector<T> &vmin, const Vector<T> &vmax) {
	this->vmin = vmin;
	this->vmax = vmax;
}

template<class T>
AABB<T>::AABB(T minx, T miny, T maxx, T maxy) {
	this->vmin.x = minx;
	this->vmin.y = miny;
	this->vmax.x = maxx;
	this->vmax.y = maxy;
}

template<class T>
AABB<T>::AABB(const Magick::Geometry &geom) {
	this->vmin.x = geom.xOff();
	this->vmin.y = geom.yOff();
	this->vmax.x = geom.width();
	this->vmax.y = geom.height();
}

template<class T>
AABB<T>::~AABB() {}

template<class T>
Vector<T> &AABB<T>::operator[](std::size_t idx) {
	switch(idx) {
		case 0: return vmin;
		case 1: return vmax;
		default: throw std::out_of_range("AABB vertex index out of range.");
	}
}

template<>
AABB<int> AABB<int>::buffer(float buf_pixels) {
	return AABB<int>(
		(int) round(vmin.x - buf_pixels),
		(int) round(vmin.y - buf_pixels),
		(int) round(vmax.x + buf_pixels),
		(int) round(vmax.y + buf_pixels)
	);
}

template<>
AABB<float> AABB<float>::buffer(float buf_pixels) {
	return AABB<float>(
		vmin.x - buf_pixels,
		vmin.y - buf_pixels,
		vmax.x + buf_pixels,
		vmax.y + buf_pixels
	);
}

template<class T>
bool AABB<T>::contains(const Vector<T> &p) const {
	if (p.x < vmin.x || p.x > vmax.x)
		return false;
	if (p.y < vmin.y || p.y > vmax.y)
		return false;
	return true;
}

template<class T>
bool AABB<T>::overlaps(const AABB<T> &aabb) const {
	if (vmax.x < aabb.vmin.x || vmin.x > aabb.vmax.x)
		return false;
	if (vmax.y < aabb.vmin.y || vmin.y > aabb.vmax.y)
		return false;
	return true;
}

template<class T>
size_t AABB<T>::size() const {
	return 2;
}

template<class T>
Polygon<T>::Polygon() {}

template<class T>
Polygon<T>::Polygon(const std::vector<Vector<T>> &poly) {
	this->v = poly;
}

template<class T>
Polygon<T>::~Polygon() {
	v.clear();
}

template<class T>
Vector<T> &Polygon<T>::operator[](std::size_t idx) {
	if (idx >= v.size())
		throw std::out_of_range("Polygon vertex index out of range.");
	return v[idx];
}

template<class T>
const Vector<T> &Polygon<T>::operator[](std::size_t idx) const {
	if (idx >= v.size())
		throw std::out_of_range("Polygon vertex index out of range.");
	return v[idx];
}

template<class T>
AABB<T> Polygon<T>::get_aabb() const {
	if (v.size() == 0)
		return AABB<T>();

	AABB<T> aabb(v[0], v[0]);
	for (size_t i=0; i<v.size(); i++) {
		if (v[i].x < aabb.vmin.x)
			aabb.vmin.x = v[i].x;
		if (v[i].y < aabb.vmin.y)
			aabb.vmin.y = v[i].y;
		if (v[i].x > aabb.vmax.x)
			aabb.vmax.x = v[i].x;
		if (v[i].y > aabb.vmax.y)
			aabb.vmax.y = v[i].y;
	}

	return aabb;
}

template<class T>
void Polygon<T>::clip_to_aabb(const AABB<T> &aabb) {
	for (size_t i=0; i<v.size(); i++) {
		if (v[i].x < aabb.vmin.x)
			v[i].x = aabb.vmin.x;
		if (v[i].y < aabb.vmin.y)
			v[i].y = aabb.vmin.y;
		if (v[i].x > aabb.vmax.x)
			v[i].x = aabb.vmax.x;
		if (v[i].y > aabb.vmax.y)
			v[i].y = aabb.vmax.y;
	}
}

template<class T>
Vector<T> Polygon<T>::center() const {
	Vector<T> c;
	for (size_t i=0; i<v.size(); i++)
		c += v[i];
	c /= (float) v.size();
	return c;
}

template<class T>
void Polygon<T>::scale(float f) {
	Vector<T> c = center();
	for (size_t i=0; i<v.size(); i++)
		v[i] = c + (v[i] - c) * f;
}

template<class T>
bool Polygon<T>::contains(const Vector<T> &p) const {
	// http://www.alienryderflex.com/polygon/
	size_t j = v.size() - 1;
	bool odd_nodes = false;

	for (size_t i=0; i<v.size(); i++) {
		if ((v[i].y < p.y && v[j].y >= p.y)
			|| (v[j].y < p.y && v[i].y >= p.y)) {
			if (v[i].x + (p.y - v[i].y) / (v[j].y - v[i].y) * (v[j].x - v[i].x) < p.x)
				odd_nodes = !odd_nodes;
		}
		j = i;
	}
	return odd_nodes;
}

template<class T>
void Polygon<T>::push_back(const Vector<T> &p) {
	v.push_back(p);
}

template<class T>
bool Polygon<T>::remove(std::size_t idx) {
	if (idx < v.size()) {
		v.erase(v.begin() + idx);
		return true;
	}
	return false;
}

template<class T>
void Polygon<T>::clear() {
	v.clear();
}

template<class T>
size_t Polygon<T>::size() const {
	return v.size();
}

template<class T>
double Polygon<T>::area() const {
	// https://stackoverflow.com/a/451482/1692112
	double area = 0.0;
	for (size_t i=0, j=0; i<size(); i++) {
		if (i == size() - 1)
			j = 0;
		else
			j = i + 1;
		// Skip degenerate edges.
		if ((v[j] - v[i]).length() > f_epsilon)
			area += cross_product(v[i], v[j]);
	}
	return 0.5 * abs(area);
}

template<class T>
std::ostream &operator<<(std::ostream &os, const Polygon<T> &poly) {
	os << "Polygon(";
	for (size_t i=0; i<poly.size(); i++) {
		os << poly[i];
		if (i < poly.size() - 1)
			os << ", ";
	}
	os << ")";
	return os;
}

std::vector<Vector<int>> extract_coords(std::string const &text, char delim, char coord_delim) {
	std::vector<Vector<int>> coords;
	std::vector<std::string> tokens = split_str(text, delim);

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++) {
		std::vector<std::string> subtokens = split_str(*it, coord_delim);
		if (subtokens.size() < 2 || subtokens[0].empty() || subtokens[1].empty())
			break;
		coords.push_back(Vector<int>(std::atoi(subtokens[0].c_str()), std::atoi(subtokens[1].c_str())));
	}

	return coords;
}

OGRGeometry *wkt_to_geom(const std::string &wkt, OGRGeometry **p_geom) {
	// Expecting a WKT string as follows:
	//   SRID=4326;Polygon ((22.64992375534184887 50.27513740160615185, 23.60228115218003708 50.35482161490517683, 23.54514084707420452 49.94024031630130622, 23.3153953947536472 50.21771699530808775, 22.64992375534184887 50.27513740160615185))

	OGRErr err = OGRERR_NONE;

	// Extract the WKT.
	std::string s_wkt;
	std::size_t i_wkt = wkt.find_last_of(';');
	if (i_wkt == std::string::npos)
		s_wkt = wkt.substr(0);
	else
		s_wkt = wkt.substr(i_wkt + 1);

	// Extract the SRID.
	int srid = -1;
	std::size_t i_srid = wkt.find_first_of('=');
	if (i_srid != std::string::npos)
		srid = atoi(wkt.substr(i_srid + 1, i_wkt - i_srid).c_str());

	//! \note We'll need to copy the pointer for OGRGeometryFactory::createFromWkt changes the pointer address.
	char *c_wkt = (char *) s_wkt.c_str();

	OGRSpatialReference *p_srs = NULL;
	if (srid > 0) {
		p_srs = new OGRSpatialReference();
		err = p_srs->importFromEPSG(srid);
		if (err != OGRERR_NONE)
			throw GDALOGRException("Failed to import spatial reference from EPSG", err);
		err = p_srs->Validate();
		if (err != OGRERR_NONE)
			throw GDALOGRException("Failed to validate spatial reference", err);
	}

	err = OGRGeometryFactory::createFromWkt(&c_wkt, p_srs, p_geom);
	if (err != OGRERR_NONE)
		throw GDALOGRException("Failed to create geometry from WKT", err);

	if (p_geom != nullptr)
		return *p_geom;
	return nullptr;
}

template<class T>
Polygon<T> proj_coords_to_raster(const OGRGeometry *p_geom, GDALDataset *p_dataset) {
	Polygon<T> poly;
	CPLErr cplerr = CE_None;
	OGRErr ogrerr = OGRERR_NONE;

	// Get spatial reference from the dataset.
	OGRSpatialReference *p_srs_r = new OGRSpatialReference(p_dataset->GetProjectionRef());
	// Get geotransform parameters from the dataset.
	double p_geo_tf[6];
	cplerr = p_dataset->GetGeoTransform(p_geo_tf);
	if (cplerr != CE_None)
		throw GDALCPLException("Failed to get geotransform from the dataset");

	// Coordinate transform from the supplied geometry to the dataset spatial reference.
	OGRCoordinateTransformation *p_ct = OGRCreateCoordinateTransformation(p_geom->getSpatialReference(), p_srs_r);
	if (!p_ct)
		throw GDALOGRException("Failed to create a coordinate transformation", ogrerr);

	// Walk through the points in the polygon.
	if (wkbFlatten(p_geom->getGeometryType()) == wkbPolygon) {
		OGRPoint pt;
		double px, py;
		Vector<T> v;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,3,0)
		const OGRPolygon *p_poly = nullptr;
		p_poly = p_geom->toPolygon();
		const OGRLinearRing *p_ext = p_poly->getExteriorRing();
#else
		OGRPolygon *p_poly = nullptr;
		p_poly = (OGRPolygon *) p_geom;
		OGRLinearRing *p_ext = p_poly->getExteriorRing();
#endif

		for (int i=0; i<p_ext->getNumPoints(); i++) {
			p_ext->getPoint(i, &pt);
			px = pt.getX();
			py = pt.getY();
			// Transform into the dataset spatial reference.
			if (!p_ct->Transform(1, &px, &py)) {
				throw GDALOGRException("Failed to transform coordinates", ogrerr);
			} else {
				v.from_geo_coords(p_geo_tf, px, py);
				poly.push_back(v);
			}
		}
	}

	if (p_srs_r != nullptr)
		delete p_srs_r;
	return poly;
}

std::vector<std::vector<unsigned char>> fill_poly_overlap(Polygon<int> &poly, float pixel_size_div) {
    // Inspired by
    //   http://alienryderflex.com/polygon_fill/
    int i, j;
    float xf;
    std::vector<int> node_x;
    Vector<int> pixel;
    Vector<float> local_a, local_b;

    AABB<int> poly_aabb = poly.get_aabb();
    Vector<int> poly_dim = poly_aabb.vmax - poly_aabb.vmin;
    // Axis-aligned bounding box in the local reference frame.
    AABB<int> local_aabb(
        Vector<int>(0, 0),
        Vector<int>(ceil(((float) poly_dim.x) / pixel_size_div),
            ceil(((float) poly_dim.y) / pixel_size_div))
    );

    // Initialize the subtile mask.
    std::vector<std::vector<unsigned char>> subtile_mask(local_aabb.vmax.x, std::vector<unsigned char>(local_aabb.vmax.y, 0));

    for (pixel.y=local_aabb.vmin.y; pixel.y < local_aabb.vmax.y; pixel.y++) {
        node_x.clear();
        // Build a list of nodes.
        j = poly.size() - 1;
        for (i=0; i<(int)poly.size(); i++) {
            local_a.x = ((float) poly[i].x - poly_aabb.vmin.x) / pixel_size_div;
            local_a.y = ((float) poly[i].y - poly_aabb.vmin.y) / pixel_size_div;
            local_b.x = ((float) poly[j].x - poly_aabb.vmin.x) / pixel_size_div;
            local_b.y = ((float) poly[j].y - poly_aabb.vmin.y) / pixel_size_div;

            if ((local_a.y < (float) pixel.y && local_b.y >= (float) pixel.y)
                || (local_b.y < (float) pixel.y && local_a.y >= (float) pixel.y)) {
                // Calculate the x-coordinate of the intersection between AB and y = pixel.y.
                xf = local_a.x + (pixel.y - local_a.y) / (local_b.y - local_a.y) * (local_b.x - local_a.x);
                node_x.push_back((int) xf);
            }
            j = i;
        }

        // Sort the nodes.
        std::sort(node_x.begin(), node_x.end());

        // Fill pixels between node pairs.
        for (i=0; i<(int)node_x.size(); i+=2) {
            if (node_x[i] >= local_aabb.vmax.x)
                break;
            if (node_x[i + 1] >= local_aabb.vmin.x) {
                if (node_x[i] < local_aabb.vmin.x)
                    node_x[i] = local_aabb.vmin.x;
                if (node_x[i + 1] > local_aabb.vmax.x)
                    node_x[i + 1] = local_aabb.vmax.x;
                for (pixel.x=node_x[i]; pixel.x<=node_x[i + 1]; pixel.x++) {
                    // Fill the pixel covered by the polygon.
                    subtile_mask[pixel.x][pixel.y] = 1;
                    // Also fill the immediate surrounding pixels.
                    if (pixel.x > 0)
                        subtile_mask[pixel.x-1][pixel.y] = 1;
                    if (pixel.x < local_aabb.vmax.x)
                        subtile_mask[pixel.x+1][pixel.y] = 1;
                    if (pixel.y > 0)
                        subtile_mask[pixel.x][pixel.y-1] = 1;
                    if (pixel.y < local_aabb.vmax.y)
                        subtile_mask[pixel.x][pixel.y+1] = 1;
                }
            }
        }
    }
    return subtile_mask;
}

std::vector<std::vector<unsigned char>> fill_poly_overlap(const AABB<int> &image_aabb, Polygon<int> &poly, float pixel_size_div, bool buffer_out) {
	// Inspired by
	//   http://alienryderflex.com/polygon_fill/
	int i, j;
	float xf;
	const float epsilon = 1E-3;
	std::vector<int> node_x;
	Vector<int> pixel;
	Vector<float> local_a, local_b;

	Vector<int> img_dim = image_aabb.vmax - image_aabb.vmin;
	// Axis-aligned bounding box for the image.
	AABB<int> local_aabb(
		Vector<int>(0, 0),
		Vector<int>(ceil(((float) img_dim.x) / pixel_size_div),
			ceil(((float) img_dim.y) / pixel_size_div))
	);

	// Initialize the subtile mask.
	std::vector<std::vector<unsigned char>> subtile_mask(local_aabb.vmax.x, std::vector<unsigned char>(local_aabb.vmax.y, 0));

	for (pixel.y=local_aabb.vmin.y; pixel.y < local_aabb.vmax.y; pixel.y++) {
		node_x.clear();
		// Build a list of nodes.
		j = poly.size() - 1;
		for (i=0; i<(int)poly.size(); i++) {
			local_a.x = ((float) poly[i].x) / pixel_size_div;
			local_a.y = ((float) poly[i].y) / pixel_size_div;
			local_b.x = ((float) poly[j].x) / pixel_size_div;
			local_b.y = ((float) poly[j].y) / pixel_size_div;

			if ((local_a.y <= (float) pixel.y && local_b.y >= (float) pixel.y)
				|| (local_b.y <= (float) pixel.y && local_a.y >= (float) pixel.y)) {
				// Calculate the x-coordinate of the intersection between AB and y = pixel.y.
				xf = local_a.x + (pixel.y - local_a.y) / (local_b.y - local_a.y + epsilon) * (local_b.x - local_a.x + epsilon);
				node_x.push_back((int) xf);
			} else if (pixel.y == (int) local_a.y && (int) local_a.y == (int) local_b.y) {
				if (local_a.x < local_b.x) {
					node_x.push_back((int) local_a.x);
					node_x.push_back((int) local_b.x);
				} else {
					node_x.push_back((int) local_b.x);
					node_x.push_back((int) local_a.x);
				}
			}
			j = i;
		}

		// Sort the nodes.
		std::sort(node_x.begin(), node_x.end());

		// Fill pixels between node pairs.
		for (i=0; i<(int)node_x.size(); i+=2) {
			if (node_x[i] >= local_aabb.vmax.x)
				break;
			if (node_x[i + 1] >= local_aabb.vmin.x) {
				if (node_x[i] < local_aabb.vmin.x)
					node_x[i] = local_aabb.vmin.x;
				if (node_x[i + 1] > local_aabb.vmax.x)
					node_x[i + 1] = local_aabb.vmax.x;
				for (pixel.x=node_x[i]; pixel.x<=node_x[i + 1]; pixel.x++) {
					// Fill the pixel covered by the polygon.
					subtile_mask[pixel.x][pixel.y] = 1;
					// Also fill the immediate surrounding pixels.
					if (buffer_out) {
						if (pixel.x > 0)
							subtile_mask[pixel.x-1][pixel.y] = 1;
						if (pixel.x + 1 < local_aabb.vmax.x)
							subtile_mask[pixel.x+1][pixel.y] = 1;
						if (pixel.y > 0)
							subtile_mask[pixel.x][pixel.y-1] = 1;
						if (pixel.y + 1 < local_aabb.vmax.y)
							subtile_mask[pixel.x][pixel.y+1] = 1;
					}
				}
			}
		}
	}
	return subtile_mask;
}

std::vector<std::vector<unsigned char>> fill_whole(const AABB<int> &image_aabb, float tile_size_div, unsigned char value) {
	std::vector<Vector<int>> subtiles;
	Vector<int> p;

	Vector<int> img_dim = image_aabb.vmax - image_aabb.vmin;
	// Axis-aligned bounding box in the local reference frame.
	AABB<int> local_aabb(
		Vector<int>(0, 0),
		Vector<int>(ceil(((float) img_dim.x) / tile_size_div),
			ceil(((float) img_dim.y) / tile_size_div))
	);

	// Initialize the subtile mask.
	std::vector<std::vector<unsigned char>> subtile_mask(local_aabb.vmax.x, std::vector<unsigned char>(local_aabb.vmax.y, value));

	return subtile_mask;
}

std::vector<std::vector<unsigned char>> apply_mask(const std::vector<std::vector<unsigned char>> &mask_in, const std::vector<std::vector<unsigned char>> &mask_to_apply) {
	std::vector<std::vector<unsigned char>> mask_out(mask_in);
	unsigned int i, j;

	for (i=0; i<mask_in.size(); i++) {
		for (j=0; j<mask_in[0].size(); j++) {
			mask_out[i][j] = mask_in[i][j] * mask_to_apply[i][j];
		}
	}

	return mask_out;
}

// Explicit template instantiation:
template class Vector<int>;
template class Vector<float>;
template class AABB<int>;
template class AABB<float>;
template class Polygon<int>;
template class Polygon<float>;
template Polygon<int> proj_coords_to_raster(const OGRGeometry *p_geom, GDALDataset *p_dataset);
template Polygon<float> proj_coords_to_raster(const OGRGeometry *p_geom, GDALDataset *p_dataset);
