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
from pathlib import Path
from multiprocessing import Process, Queue

import utilities as utilities
from logger import Logger


class Uploader(Logger):
    def __init__(self, product_reader, data_dir):
        super(Uploader, self).__init__(data_dir)
        self.product_reader = product_reader
        self.n_threads = 10
        self.host = "hpc_cloudmask@rocket.hpc.ut.ee"
        self.remote_data_dir = "bands"

    def start(self):
        product_titles = self.product_reader.get_products()
        n_products = len(product_titles)
        if n_products == 0:
            self.info('Nothing to upload.')
            return

        self.info("Uploading {} products(s) with {} processes(s)".format(n_products, self.n_threads))

        try:
            self.create_remote_dir(self.host, self.remote_data_dir)
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
            processes = [Process(target=self.upload, args=args) for _ in range(self.n_threads)]
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
            band_file_prefix = "{}_{}_".format(split_title[-2], split_title[-1])

            tile_dirs = [f.name for f in os.scandir(product_path) if f.is_dir()]
            self.info("Uploading {} tiles of product {}".format(len(tile_dirs), product_title))

            for tile_dir in tile_dirs:
                destination_file = band_file_prefix + tile_dir + ".nc"
                from_path = os.path.join(product_path, tile_dir, "bands.nc")
                to_path = "{host}:{dir}/{file}".format(host=self.host, dir=self.remote_data_dir, file=destination_file)
                command = "scp {} {}".format(from_path, to_path)
                error_code, error_msg = utilities.execute(command)
                if error_code != 0:
                    self.info("Failed to upload data to {}. Error code: {}, error message:\n{}"
                              .format(to_path, error_code, error_msg))

    @staticmethod
    def create_remote_dir(host, remote_data_dir):
        command = "mkdir -p " + remote_data_dir
        command = 'ssh -o "StrictHostKeyChecking=no" {host} "{cmd}"'.format(host=host, cmd=command)
        error_code, error_msg = utilities.execute(command)
        if error_code != 0:
            raise RuntimeError("Failed to create data directory {} to {}; Error code: {}, error message:\n{}"
                               .format(remote_data_dir, host, error_code, error_msg))
