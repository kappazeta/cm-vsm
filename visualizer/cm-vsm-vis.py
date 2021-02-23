#!/bin/python3

# Tool to visualize NetCDF files in a subtiled Sentinel-2 product.
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
import argparse
import numpy as np
from PIL import Image, ImageDraw
from netCDF4 import Dataset


class CMVSMVis(object):
    def __init__(self):
        self.tile_w = 0
        self.tile_h = 0
        self.max_tile_x = 0
        self.max_tile_y = 0
        self.image = None

    @staticmethod
    def get_nc_info(fpath):
        """
        Get info on a NetCDF file.
        :param fpath: Path to the NetCDF file.
        :return: Dictionary with width, height, and list of bands
        """
        root = Dataset(fpath, "r")
        w = root.dimensions["x"].size
        h = root.dimensions["y"].size
        bands = list(root.variables.keys())
        root.close()
        return {
            "width": w,
            "height": h,
            "bands": bands
        }

    @staticmethod
    def get_coords_from_fpath(fpath):
        """
        Get subtile coordinates from a NetCDF path.
        :param fpath: NetCDF filepath.
        :return: x, y
        """
        m = re.match(r".*tile_(\d+)_(\d+)/bands.nc", fpath)
        if m:
            x, y = int(m.group(1)), int(m.group(2))
            return x, y
        return None, None

    @staticmethod
    def get_subtile_band(fpath, band):
        """
        Get an 8-bit grayscale image of a subtile band.
        :param fpath: Path to the subtile NetCDF.
        :param band: Name of the band.
        :return: Numpy array of the subtile.
        """
        root = Dataset(fpath, "r")
        band_data = root["/" + band][:]
        v_min = np.nanmin(band_data)
        v_max = np.nanmax(band_data)
        if v_min == v_max:
            band_data.fill(v_min)
        else:
            band_data = 255 * (band_data - v_min) / (v_max - v_min)
        root.close()
        # Flip the rows, to have subtile contents upright.
        return np.flip(band_data.astype(np.uint8), 0)

    def blit_subtile(self, fpath, band):
        """
        Blit a subtile onto the image.
        :param fpath: Path to the NetCDF file to blit.
        :param band: Name of the band to blit.
        :return: Modified image.
        """
        tile_x, tile_y = self.get_coords_from_fpath(fpath)

        band_data = self.get_subtile_band(fpath, band)

        x0 = tile_x * self.tile_w
        x1 = (tile_x + 1) * self.tile_w
        y0 = tile_y * self.tile_h
        y1 = (tile_y + 1) * self.tile_h
        self.image[y0:y1, x0:x1] = band_data

        return self.image

    def blit_subtiles(self, dpath, band):
        """
        Blit the specified band of all subtiles in a directory.
        :param dpath: Directory path.
        :param band: Name of the band.
        :return: The composite image.
        """
        fpaths = []

        # Map reduced product names to shapefile paths.
        for root, dirs, files in os.walk(dpath):
            nc_path = os.path.join(root, "bands.nc")
            if os.path.exists(nc_path):
                fpaths.append(nc_path)

                x, y = self.get_coords_from_fpath(nc_path)
                if x > self.max_tile_x:
                    self.max_tile_x = x
                if y > self.max_tile_y:
                    self.max_tile_y = y

        # Initialize the image.
        if len(fpaths) > 0:
            # Get tile size and list of bands from the first NetCDF.
            d = self.get_nc_info(fpaths[0])
            self.tile_w, self.tile_h = d["width"], d["height"]

            # Prepare an empty image.
            img_w = (self.max_tile_x + 1) * self.tile_w
            img_h = (self.max_tile_y + 1) * self.tile_h
            self.image = np.zeros((img_w, img_h), dtype=np.uint8)

            # Blit subtiles.
            for fpath in fpaths:
                self.blit_subtile(fpath, band)

        return self.image


def main():
    parser = argparse.ArgumentParser(description='Visualize the contents of a subtiled product')
    parser.add_argument('cm_path', type=str, help='Path to the directory with the subtiled product (.CVAT directory)')
    parser.add_argument('-b', '--band', type=str, help='Name of the band to visualize', default="B09")
    args = parser.parse_args()

    vis = CMVSMVis()
    vis.blit_subtiles(args.cm_path, args.band)

    image = Image.fromarray(vis.image)

    # Draw subtile names on the image.
    draw = ImageDraw.Draw(image)
    for x in range(vis.max_tile_x):
        for y in range(vis.max_tile_y):
            text = "tile_{}_{}".format(x, y)
            draw.text((x * vis.tile_w, y * vis.tile_h), text, 255)

    image.show()


if __name__ == "__main__":
    main()

