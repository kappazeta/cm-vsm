# Parallel Sentinel-2 downloader for DIAS
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

import s3 as s3
import utilities as utilities
from logger import Logger


class Downloader(Logger):
    def __init__(self, product_reader, data_dir):
        super(Downloader, self).__init__(data_dir)
        self.n_threads = 2
        self.product_reader = product_reader
        self.s3_bucket_name = "EODATA"
        self.disk_quota_str = "200G"
        self.disk_quota = utilities.size_from_str(self.disk_quota_str)

    def start(self):
        product_titles = self.product_reader.get_products()
        n_products = len(product_titles)
        if n_products == 0:
            self.info('Nothing to download.')
            return

        self.info("Downloading {} products(s) with {} processes(s)".format(n_products, self.n_threads))

        jobs_queue = Queue()
        [jobs_queue.put(product) for product in product_titles]
        control_queue = Queue()
        control_queue.put(-1)

        # In case of Ctrl+C, send a cancel signal to all processes so that they could return
        try:
            args = [control_queue, jobs_queue]
            processes = [Process(target=self.download, args=args) for _ in range(self.n_threads)]
            [process.start() for process in processes]
            [process.join() for process in processes]
        except KeyboardInterrupt:
            # Cause the threads to return
            while not control_queue.empty():
                control_queue.get()
        finally:
            [process.join() for process in processes]

        self.info("Finished downloading.")

    def download(self, control_queue, jobs_queue):
        # In case of ctrl+c, control_queue will become empty
        while (not control_queue.empty()) and (not jobs_queue.empty()):
            product_title = jobs_queue.get()
            product_path = s3.get_path(product_title)
            download_path = os.path.join(self.data_dir, product_title + ".SAFE.part")
            download_size = self.get_download_size(product_title, product_path, download_path)
            if download_size < 0:
                continue

            dir_size = utilities.get_dir_size(self.data_dir) + download_size
            if dir_size > self.disk_quota:
                self.info("Skipping product {} as disk is full: {}/{}"
                          .format(product_title, utilities.size_to_str(dir_size), self.disk_quota_str))
                continue

            if not Path(download_path).is_dir():
                os.makedirs(download_path)

            try:
                command = s3.get_command(self.s3_bucket_name, product_path, download_path)
                self.info("Downloading product {} as {}".format(product_title, command))
                self.download_product(command, download_size, download_path, product_title)
            except RuntimeError as error:
                self.info(str(error))

    @staticmethod
    def download_product(s3_command, download_size, download_path, product_title):
        error_code, error_msg = utilities.execute(s3_command)
        if error_code != 0:
            raise RuntimeError("Failed to download product {}. Error code: {}, error message:\n{}"
                               .format(product_title, error_code, error_msg))

        # Check that the products were downloaded correctly.
        local_size = -1
        if Path(download_path).is_dir():
            local_size = utilities.get_dir_size(download_path)
        if download_size != local_size:
            raise RuntimeError("Failed to verify product {}. {}.local != {}.remote"
                               .format(product_title, utilities.size_to_str(local_size), utilities.size_to_str(download_size)))

        # Delete already existing but non-complete product directory
        result_path = download_path.replace(".part", "")
        if Path(result_path).is_dir():
            shutil.rmtree(result_path)

        # Remove .part from the dir name, so that other modules can start using it
        shutil.move(download_path, result_path)

    def get_download_size(self, product_title, product_path, download_path):
        try:
            # Get the size of the download.
            download_size = s3.get_size(self.s3_bucket_name, product_path)
            size_str = utilities.size_to_str(download_size)
            download_path = download_path.replace(".part", "")
            if Path(download_path).is_dir():
                local_size = utilities.get_dir_size(download_path)
                # In case product with the same size already exists, do not download it again
                if download_size == local_size:
                    self.info("Product {} already exists and occupies {}".format(product_title, size_str))
                    return -1
            else:
                self.info("Size of product {} is {}".format(product_title, size_str))
            return download_size

        except Exception as e:
            self.info("Failed to obtain size of product {}".format(product_title))
            return -1
