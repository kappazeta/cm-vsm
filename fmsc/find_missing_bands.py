#!/bin/python3

# Script to check if the input Sentinel-2 products have all the necessary bands.
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

EXPECTED_BANDS = ['B01', 'B02', 'B03', 'B04', 'B05', 'B06', 'B07', 'B08', 'B8A', 'B09', 'B10', 'B11', 'B12', 'TCI']


def main():
    parser = argparse.ArgumentParser(description='Find incomplete products')
    parser.add_argument('safe_path', type=str, help='Path to the directory with SAFE products')
    args = parser.parse_args()

    incomplete = {}

    # Map reduced product names to shapefile paths.
    for root, dirs, files in os.walk(args.safe_path):
        if root.endswith("IMG_DATA"):
            for band in EXPECTED_BANDS:
                is_present = False
                for fname in files:
                    if band in fname:
                        is_present = True
                        break

                if not is_present:
                    if root in incomplete:
                        incomplete[root].append(band)
                    else:
                        incomplete[root] = [band]

    for key, val in incomplete.items():
        print('{}\t\t{}'.format(key, val))


if __name__ == "__main__":
    main()


