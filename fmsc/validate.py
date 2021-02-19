#!/bin/python3

# Script to check the sizes of TCI PNG in subtile directories.
# Outputs a table of Sentinel-2 product name, mean file size in KiB, standard deviation in KiB.
# Small file sizes with 0 deviation should indicate either fully clouded or invalid products.
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
import argparse
import subprocess
import numpy as np
from pathlib import Path


def get_product_name_from_path(path):
    parents = list(Path(path).parents)
    for p in parents:
        if p.suffix == '.CVAT':
            return p.stem


def get_product_name_from_fname(fname):
    return Path(fname).stem


def main():
    parser = argparse.ArgumentParser(description='Validate content in .CVAT directories')
    parser.add_argument('cvat_path', type=str, help='Path to the directory with CVAT products')
    args = parser.parse_args()

    product_tci_sizes = {}

    # Map reduced product names to shapefile paths.
    for root, dirs, files in os.walk(args.cvat_path):
        product_name = get_product_name_from_path(root)
        if product_name not in product_tci_sizes:
            product_tci_sizes[product_name] = []

        for fname in files:
            if fname.lower().endswith(".png") and "_TCI_" in fname:
                fpath = os.path.join(root, fname)
                fsize = os.path.getsize(fpath)

                product_tci_sizes[product_name].append(fsize)

    product_tci_statistics = []

    for product_name, sizes in product_tci_sizes.items():
        fs_avg = np.mean(sizes)
        fs_std = np.std(sizes)
        product_tci_statistics.append((product_name, fs_avg, fs_std))

    product_tci_statistics = sorted(product_tci_statistics, key=lambda x: x[1])
    for product_name, fs_avg, fs_std in product_tci_statistics:
        print("{}:\t{:.2f}\t{:.2f}".format(product_name, fs_avg / 1024.0, fs_std / 1024.0))


if __name__ == "__main__":
    main()

