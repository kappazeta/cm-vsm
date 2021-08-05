#!/bin/python3

# Sentinel-2 product preprocessor for the NASA GSFC dataset.
# https://data.mendeley.com/datasets/r7tnvx7d9g/1
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
import json
import os
import argparse
import subprocess
import re
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


def label_rasterize(sample_band_path, shapefile_path_in, raster_path_out):
    gdai_info_output = (subprocess.run(["gdalinfo", sample_band_path], capture_output=True).stdout).decode('utf-8')
    pattern = r"Size\sis\s([0-9]+),\s([0-9]+)"
    match = re.findall(pattern, gdai_info_output)

    # gdal_rasterize -a class -ot Byte -ts x y input.shp output.tif
    new_name = os.path.join(raster_path_out[:raster_path_out.rfind('/')],'label.tif')
    subprocess.run(["gdal_rasterize", "-a", "class", "-ot", "Byte", "-ts", str(match[0][0]), str(match[0][1]), shapefile_path_in, new_name])


def main():
    parser = argparse.ArgumentParser(description='Crop S2 products by shapefiles.')
    parser.add_argument('s2_path', type=str, help='Path to the directory with S2 products')
    parser.add_argument('out_path', type=str, help='Path to the output directory')
    args = parser.parse_args()

    # Look through the specified folder, find .SAFE and process them one by one
    for entry in os.listdir(args.s2_path):
        if '.SAFE' in entry:
            shapefile = ""
            sample_band = ""
            abs_path = os.path.join(args.s2_path, entry)
            for root, dirs, files in os.walk(abs_path):
                out_path = os.path.join(args.out_path, os.path.relpath(root, args.s2_path))
                # Create subdirectories in the output directory.
                Path(out_path).mkdir(parents=True, exist_ok=True)
                # Save the path for the border layer
                for fname in files:
                    if fname.endswith("border.shp"):
                        shapefile = os.path.join(root, fname)
                        break

            # Find rasters and .gml files and crop them accordingly to border.shp
            for root, dirs, files in os.walk(abs_path):
                for fname in files:
                    out_path = os.path.join(args.out_path, os.path.relpath(root, args.s2_path))
                    fpath = os.path.join(root, fname)
                    out_fpath = os.path.join(out_path, fname)
                    if fname.lower().endswith(".jp2"):
                        print("Cropping {}".format(fname))
                        crop_jp2(fpath, shapefile, out_fpath)
                        # For rasterizing at 10m resulution B02 for L1C and B02_10m for L2A products are used
                        if "B02.jp2" in fname or "B02_10m.jp2" in fname:
                            sample_band = out_fpath
                    elif fname.lower().endswith(".gml"):
                        print("Cropping {}".format(fname))
                        crop_gml(fpath, shapefile, out_fpath)

                # Rasterizing itself
                for fname in files:
                    if 's2' in fname and fname.lower().endswith(".shp"):
                        out_path = os.path.join(args.out_path, os.path.relpath(root, args.s2_path))
                        out_fpath = os.path.join(out_path, fname)
                        label_rasterize(sample_band,os.path.join(root,fname),out_fpath )


if __name__ == "__main__":
    main()
