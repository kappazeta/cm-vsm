# CVAT data uploader for DIAS
#
# Copyright 2020 KappaZeta Ltd.
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
import shutil
from pathlib import Path

import utilities as utilities
from logger import Logger


class Uploader(Logger):
    def __init__(self, product_reader, data_dir):
        super(Uploader, self).__init__(data_dir)
        self.product_reader = product_reader
        host = "hpc_cloudmask@rocket.hpc.ut.ee"
        remote_data_dir = ""  # home directory
        self.remote_data_path = "{}:{}".format(host, remote_data_dir)
        self.path_to_bands = os.path.join(self.data_dir, "bands")

    def start(self):
        product_titles = self.product_reader.get_products()
        n_products = len(product_titles)
        if n_products == 0:
            self.info('Nothing to upload.')
            return

        self.info("Uploading {} products(s)".format(n_products))

        # Clear all the previous contents of the folder that will be uploaded
        if Path(self.path_to_bands).is_dir():
            shutil.rmtree(self.path_to_bands)
        os.makedirs(self.path_to_bands)

        self.compose_upload_data(product_titles)

        copy_command = "rsync -a {} {}".format(self.path_to_bands, self.remote_data_path)
        error_code, error_msg = utilities.execute(copy_command)
        if error_code != 0:
            self.info("Failed to upload data to {}. Error code: {}, error message:\n{}"
                      .format(self.remote_data_path, error_code, error_msg))

        self.info("Finished uploading.")

    def compose_upload_data(self, product_titles):
        for product_title in product_titles:
            product_path = os.path.join(self.data_dir, product_title + ".CVAT")

            if not Path(product_path).is_dir():
                self.log("No .CVAT directory for product {}".format(product_title))
                continue

            split_title = product_title.split("_")
            band_file_prefix = "{}_{}_".format(split_title[-2], split_title[-1])

            tile_dirs = [f.name for f in os.scandir(product_path) if f.is_dir()]
            for tile_dir in tile_dirs:
                from_path = os.path.join(product_path, tile_dir, "bands.nc")
                to_path = os.path.join(self.path_to_bands, band_file_prefix + tile_dir + ".nc")
                shutil.copy(from_path, to_path)
