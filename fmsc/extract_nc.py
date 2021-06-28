#!/bin/python3

# Script to collect NetCDF files from processed Sentinel-2 products.
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
import subprocess
import numpy as np
from pathlib import Path
from shutil import copyfile


def build_name(fpath):
    """
    Prepare a filename for the NetCDF file.
    """
    parents = fpath.split("/")
    tile_name = parents[-2]
    product_name = None
    m = re.match(r"S2(?:A|B)_MSIL(?:1|2)(?:A|C)_[\dT]+_N\d+_R\d+_([0-9A-Z]+_[\dT]+).CVAT$", parents[-3])
    if m:
        product_name = m.group(1)
    if None not in [tile_name, product_name]:
        return "{}_{}.nc".format(product_name, tile_name)


def main():
    parser = argparse.ArgumentParser(description='Extract NetCDF files from .CVAT directories')
    parser.add_argument('cvat_path', type=str, help='Path to the directory with CVAT products')
    parser.add_argument('out_path', type=str, help='Path to the directory to store the NetCDF files in')
    args = parser.parse_args()

    Path(args.out_path).mkdir(parents=True, exist_ok=True)

    # Map reduced product names to shapefile paths.
    for root, dirs, files in os.walk(args.cvat_path):
        for fname in files:
            if fname.lower().endswith(".nc"):
                fpath = os.path.join(root, fname)
                fname2 = build_name(fpath)
                print("Copying {}".format(fname2))
                fpath_out = os.path.join(args.out_path, fname2)
                copyfile(fpath, fpath_out)


if __name__ == "__main__":
    main()

