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
from multiprocessing import Process, Queue

import utilities as utilities
from logger import Logger


class Uploader(Logger):
    def __init__(self, product_reader, data_dir):
        super(Uploader, self).__init__(data_dir)
        self.product_reader = product_reader
        self.max_n_threads = 10
        self.host = "hpc_cloudmask@rocket.hpc.ut.ee"
        self.remote_data_dir = "bands"

    def start(self):
        product_titles = self.product_reader.get_products()
        n_products = len(product_titles)
        if n_products == 0:
            self.info('Nothing to upload.')
            return

        n_threads = min(self.max_n_threads, n_products)

        self.info("Uploading {} products(s) with {} processes(s)".format(n_products, n_threads))

        try:
            self.run_remote_command(self.host, "mkdir -p " + self.remote_data_dir)
        except RuntimeError as e:
            self.info(str(e))
            return

        jobs_queue = Queue()
        [jobs_queue.put(product) for product in product_titles]
        control_queue = Queue()
        control_queue.put(-1)

        # In case of Ctrl+C, send a cancel signal to all processes so that they could return
        try:
            args = [control_queue, jobs_queue]
            processes = [Process(target=self.upload, args=args) for _ in range(n_threads)]
            [process.start() for process in processes]
            [process.join() for process in processes]
        except KeyboardInterrupt:
            # Cause the threads to return
            while not control_queue.empty():
                control_queue.get()
        finally:
            [process.join() for process in processes]

        self.info("Finished uploading.")

    def upload(self, control_queue, jobs_queue):
        # In case of ctrl+c, control_queue will become empty
        while (not control_queue.empty()) and (not jobs_queue.empty()):
            product_title = jobs_queue.get()
            product_path = os.path.join(self.data_dir, product_title + ".CVAT")

            if not Path(product_path).is_dir():
                self.info("No .CVAT directory for product {}".format(product_title))
                continue

            split_title = product_title.split("_")
            band_file_prefix = "{}_{}".format(split_title[-2], split_title[-1])

            tile_dirs = [f.name for f in os.scandir(product_path) if f.is_dir()]
            self.info("Uploading {} tiles of product {}".format(len(tile_dirs), product_title))

            # Make separate empty directory for all the bands.nc files
            bands_dir_path = product_path.replace(".CVAT", ".BANDS")
            if Path(bands_dir_path).is_dir():
                shutil.rmtree(bands_dir_path)
            os.makedirs(bands_dir_path)

            # Copy all the bands.nc files into a separate directory
            for tile_dir in tile_dirs:
                destination_file = "{}_{}.nc".format(band_file_prefix, tile_dir)
                from_path = os.path.join(product_path, tile_dir, "bands.nc")
                to_path = os.path.join(bands_dir_path, destination_file)
                shutil.copy(from_path, to_path)

            # Upload this directory to a remote host
            from_path = bands_dir_path
            to_path = "{host}:{dir}/{bands}".format(host=self.host, dir=self.remote_data_dir, bands=product_title)

            try:
                self.send_to_remote(from_path, to_path)
                self.run_remote_command(self.host, "mv {dir1}/{dir2}/* {dir1}/ && rm -rf {dir1}/{dir2}"
                                        .format(dir1=self.remote_data_dir, dir2=product_title))
                self.info("Sent {} to {}:{}".format(from_path, self.host, self.remote_data_dir))
            except RuntimeError as e:
                self.info(str(e))

    @staticmethod
    def run_remote_command(host, cmd):
        command = 'ssh -o "StrictHostKeyChecking=no" {host} "{cmd}"'.format(host=host, cmd=cmd)
        error_code, error_msg = utilities.execute(command)
        if error_code != 0:
            raise RuntimeError("Failed to run remote command {}; error code: {}, error message:\n{}"
                               .format(command, host, error_code, error_msg))

    @staticmethod
    def send_to_remote(from_path, to_path):
        command = "scp -r {} {}".format(from_path, to_path)
        error_code, error_msg = utilities.execute(command)
        if error_code != 0:
            raise RuntimeError("Failed to upload {}; error code: {}, error message:\n{}"
                               .format(from_path, error_code, error_msg))
