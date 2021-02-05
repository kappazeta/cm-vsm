#!/bin(python3

# Script to run cvat_vsm on a directory of Sentinel-2 products.
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


def subsplit(path_src):
    """
    Process a Sentinel-2 product.
    """
    subprocess.run(["cvat_vsm", "-d", path_src])


def main():
    parser = argparse.ArgumentParser(description='Subsplit all S2 products in .SAFE directories')
    parser.add_argument('s2_path', type=str, help='Path to the directory with S2 products')
    args = parser.parse_args()

    # TODO:: Turn into command-line arguments.
    run_in_parallel = False

    paths = []
    # Map reduced product names to shapefile paths.
    for root, dirs, files in os.walk(args.s2_path):
        if root.endswith(".SAFE"):
            paths.append(root)
            print(root)

            if not run_in_parallel:
                subsplit(root)

    # Run all splittings in parallel.
    if run_in_parallel:
        commands = ["cvat_vsm -d " + p for p in paths]
        procs = [Popen(p) for p in commands]
        for p in procs:
            p.wait()


if __name__ == "__main__":
    main()

