#!/bin(python3

# Sentinel-2 product preprocessor for the reference dataset by Francis, Mrziglod, Sidiropoulos et al.
# https://zenodo.org/record/4172871#.X6popcgzZaR
#
# Copyright 2021 KappaZeta Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import re
import numpy as np
import imageio as iio
import fiona
import argparse
import subprocess
from pathlib import Path


def crop_jp2(path_src, path_shp, path_dst):
    """
    Crop a JP2 raster by a shapefile.
    """
    # Since gdalwarp doesn't support JP2 output format directly, we'll save as GeoTIFF first,
    subprocess.run(["gdalwarp", "-cutline", path_shp, "-crop_to_cutline", "-of", "GTiff", "-overwrite", path_src, path_dst + ".tif"])
    # and then convert it into JP2,
    subprocess.run(["gdal_translate", "-of", "JP2OpenJPEG", path_dst + ".tif", path_dst])
    # removing the leftover TIFF file.
    subprocess.run(["rm", "-f", path_dst + ".tif"])


def crop_gml(path_src, path_shp, path_dst):
    """
    Crop a GML vector by a shapefile.
    """
    subprocess.run(["ogr2ogr", "-clipsrc", path_shp, "-f", "GML", "-overwrite", path_dst, path_src])


def get_product_name_from_path(path):
    """
    Extract S2 product name from the path.
    """
    parents = list(Path(path).parents)
    if len(parents) > 1:
        first_dir = Path(parents[len(parents) - 2])
        if first_dir.suffix == '.SAFE':
            return first_dir.stem


def get_product_name_from_fname(fname):
    """
    Get the basename from a filename based on an S2 product.
    """
    return Path(fname).stem


def reduce_product_name(product_name):
    """
    Remove volatile parts of the product name, to obtain a symmetry between L1C and L2A products.
    """
    m = re.match(r'(S2(?:A|B)_MSIL(?:1C|2A)_(?:[\dT]+))_N\d+_(R\d+_[0-9A-Z]+)_(?:[\dT]+)', product_name)
    if m:
        return "{}_{}".format(m.group(1), m.group(2))


def resize_shp(path_in, path_out, f=1.0):
    """
    In the original dataset, the shapefiles are a bit too large, covering more area than what has been labelled.
    We'll copy the shapefile and resize it around the centroid, multiplying the relative coordinates by the scaling factor f.
    """
    with fiona.open(path_in, "r") as src:
        with fiona.open(path_out, "w", crs=src.crs, driver=src.driver, schema=src.schema) as dst:
            for v in src:
                c_x, c_y = 0.0, 0.0
                num_coords = 0
                # Calculate centroid
                for p in v["geometry"]["coordinates"]:
                    # NOTE:: The last coordinates duplicate the first.
                    # This indicates the end of the polygon for GIS software.
                    num_coords += len(p) - 1
                    for i in range(len(p) - 1):
                        c_x += p[i][0]
                        c_y += p[i][1]
                c_x /= float(num_coords)
                c_y /= float(num_coords)
                # Scale around the centroid.
                for p in v["geometry"]["coordinates"]:
                    for i in range(len(p)):
                        c = p[i]
                        c = (c[0] - c_x) * f + c_x, (c[1] - c_y) * f + c_y
                        p[i] = c

                dst.write(v)


def process_npy(path_in, path_out):
    """
    Convert a Numpy array of booleans into a PNG image of 3 pixel values.
    """
    # The source file is a Numpy array of dimensions (1022, 1022, 3).
    # Three booleans per pixel: CLEAR, CLOUD, CLOUD_SHADOW.
    a = np.load(path_in)
    b = np.zeros_like(a, dtype=np.uint8)
    b[a[:, :, 0]] = 0  # CLEAR
    b[a[:, :, 1]] = 1  # CLOUD
    b[a[:, :, 2]] = 2  # CLOUD_SHADOW
    iio.imwrite(path_out, b)


def main():
    parser = argparse.ArgumentParser(description='Crop S2 products by shapefiles.')
    parser.add_argument('s2_path', type=str, help='Path to the directory with S2 products')
    parser.add_argument('out_path', type=str, help='Path to the output directory')
    # Default to a scaling factor of 0.8871527777777778, to remove the 65 pixel borders around the
    # labelled subsets of 1022 x 1022 pixels.
    parser.add_argument('--shp-scale', type=float, help='Resize factor for Shapefile', default=0.8871527777777778)
    args = parser.parse_args()

    mask_path = os.path.join(args.s2_path, 'masks')
    shp_path = os.path.join(args.s2_path, 'shapefiles')

    # TODO:: Turn into command-line arguments.
    enable_shp_rescaling = True
    enable_npy_conversion = True
    enable_jp2_cropping = True
    enable_gml_cropping = True

    # Map reduced product names to shapefile paths.
    shapefiles = {}
    for root, dirs, files in os.walk(shp_path):
        relpath = os.path.relpath(root, args.s2_path)
        out_path = os.path.join(args.out_path, relpath)
        # Create subdirectories in the output directory.
        Path(out_path).mkdir(parents=True, exist_ok=True)

        for fname in files:
            if fname.endswith(".shp"):
                fpath_in = os.path.join(root, fname)
                fpath_out = os.path.join(out_path, fname)
                # Resize the Shapefile.
                if enable_shp_rescaling:
                    print("Rescaling {}".format(fname))
                    resize_shp(fpath_in, fpath_out, f=args.shp_scale)

                basename_l1c = reduce_product_name(Path(fname).stem)
                basename_l2a = basename_l1c.replace('MSIL1C', 'MSIL2A')
                shapefiles[basename_l1c] = fpath_out
                shapefiles[basename_l2a] = fpath_out

    # Map reduced product names to S2 product paths.
    s2_products = {}
    for root, dirs, files in os.walk(args.s2_path):
        relpath = os.path.relpath(root, args.s2_path)
        out_path = os.path.join(args.out_path, relpath)
        # Create subdirectories in the output directory.
        Path(out_path).mkdir(parents=True, exist_ok=True)

        if root.endswith(".SAFE"):
            product_name = get_product_name_from_fname(relpath)
            if product_name is not None:
                reduced_product_name_l2a = reduce_product_name(product_name)
                reduced_product_name_l1c = reduced_product_name_l2a.replace('MSIL2A', 'MSIL1C')
                s2_products[reduced_product_name_l1c] = relpath
                s2_products[reduced_product_name_l2a] = relpath

    # Process files in the S2 path.
    for root, dirs, files in os.walk(args.s2_path):
        relpath = os.path.relpath(root, args.s2_path)
        out_path = os.path.join(args.out_path, relpath)

        # Convert Numpy arrays into classification maps.
        if enable_npy_conversion:
            for fname in files:
                fpath = os.path.join(root, fname)
                if root.endswith('/masks') and fname.endswith('.npy'):
                    file_rpname = reduce_product_name(get_product_name_from_fname(fname))
                    if file_rpname in s2_products.keys():
                        print("Converting {}".format(fname))
                        out_dpath = os.path.join(os.path.join(args.out_path, s2_products[file_rpname]), "ref_dataset_mrziglod20")
                        out_fpath = os.path.join(out_dpath, "classification_map.png")
                        # Create the subdirectory.
                        Path(out_dpath).mkdir(exist_ok=True)
                        # Convert Numpy array into a classification map image.
                        process_npy(fpath, out_fpath)

        product_name = get_product_name_from_path(relpath)
        if product_name is None:
            continue

        reduced_product_name = reduce_product_name(product_name)
        if reduced_product_name not in shapefiles.keys():
            print("Missing shapefile for {}".format(product_name))
            continue

        shapefile = shapefiles[reduced_product_name]

        for fname in files:
            fpath = os.path.join(root, fname)
            out_fpath = os.path.abspath(os.path.join(out_path, fname))
            if enable_jp2_cropping and fname.lower().endswith(".jp2"):
                print("Cropping {}".format(fname))
                crop_jp2(fpath, shapefile, out_fpath)
            elif enable_gml_cropping and fname.lower().endswith(".gml"):
                print("Cropping {}".format(fname))
                crop_gml(fpath, shapefile, out_fpath)


if __name__ == "__main__":
    main()
