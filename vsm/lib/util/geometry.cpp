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
#include <cstring>
// GDAL
#include <ogrsf_frmts.h>
// #include <ogr_geometry.h>
#include <ogr_spatialref.h>


FVertex::FVertex(): x(0.0f), y(0.0f) {}
FVertex::FVertex(float x, float y): x(x), y(y) {}
FVertex::~FVertex() {}

IVertex::IVertex(): x(0), y(0) {}
IVertex::IVertex(int x, int y): x(x), y(y) {}
IVertex::~IVertex() {}

IVertex geo_to_pixel_coords(const double *padfTransform, double x, double y) {
	// https://stackoverflow.com/a/58814574/1692112
	IVertex v;

	v.x = int((x - padfTransform[0]) / padfTransform[1]);
	v.y = int((y - padfTransform[3]) / padfTransform[5]);

	return v;
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

std::vector<IVertex> proj_coords_to_raster(const OGRGeometry *p_geom, GDALDataset *p_dataset) {
	std::vector<IVertex> vv;
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
		IVertex iv;

		OGRPolygon *p_poly = nullptr;
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,3,0)
		p_poly = p_geom->toPolygon();
#else
		p_poly = (OGRPolygon *) p_geom;
#endif

		OGRLinearRing *p_ext = p_poly->getExteriorRing();
		for (int i=0; i<p_ext->getNumPoints(); i++) {
			p_ext->getPoint(i, &pt);
			px = pt.getX();
			py = pt.getY();
			// Transform into the dataset spatial reference.
			if (!p_ct->Transform(1, &px, &py)) {
				throw GDALOGRException("Failed to transform coordinates", ogrerr);
			} else {
				iv = geo_to_pixel_coords(p_geo_tf, px, py);
				vv.push_back(iv);
				std::cout << "Point " << pt.getX() << ", " << pt.getY() << " -> " << px << ", " << py << " -> " << iv.x << ", " << iv.y << std::endl;
			}
		}
	}

	if (p_srs_r != nullptr)
		delete p_srs_r;
	return vv;
}

