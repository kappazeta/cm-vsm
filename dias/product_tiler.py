# Sentinel-2 product splitter, using VSM
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


class ProductTiler(Logger):
    def __init__(self, product_reader, data_dir):
        super(ProductTiler, self).__init__(data_dir)
        self.n_threads = 10
        self.product_reader = product_reader
        self.cvat_command = "../vsm/build/bin/cvat_vsm -d "

    def start(self):
        product_titles = self.product_reader.get_products()
        n_products = len(product_titles)
        if n_products == 0:
            self.info('Nothing to tile.')
            return

        self.info("Tiling {} products(s) with {} processes(s)".format(n_products, self.n_threads))

        jobs_queue = Queue()
        [jobs_queue.put(product) for product in product_titles]
        control_queue = Queue()
        control_queue.put(-1)

        # In case of Ctrl+C, send a cancel signal to all processes so that they could return
        try:
            args = [control_queue, jobs_queue]
            processes = [Process(target=self.split, args=args) for _ in range(self.n_threads)]
            [process.start() for process in processes]
            [process.join() for process in processes]
        except KeyboardInterrupt:
            # Cause the threads to return
            while not control_queue.empty():
                control_queue.get()
        finally:
            [process.join() for process in processes]

        self.info("Finished tiling.")

    def split(self, control_queue, jobs_queue):
        # In case of ctrl+c, control_queue will become empty
        while (not control_queue.empty()) and (not jobs_queue.empty()):
            product_title = jobs_queue.get()
            product_path = os.path.join(self.data_dir, product_title + ".SAFE")
            ref_data_path = os.path.join(self.data_dir, "Reference_dataset", product_title.replace("MSIL2A", "MSIL1C"))

            # Check for the existence of downloaded data
            if not Path(product_path).is_dir():
                self.info("Could not find data for product " + product_title)
                continue

            # Create reference data
            if not Path(ref_data_path).is_dir():
                self.info("Could not find reference dataset for product " + product_title)
                continue
            # Create link instead of coping, as this does not affect the size of the .SAFE dir
            # and therefore does not corrupt the check of the correctness of the dir
            utilities.execute("ln -s {} {}".format(ref_data_path, product_path + "/ref_dataset"))

            # Start the tiling process
            command = self.cvat_command + product_path
            self.info("Tiling product {} with command {}".format(product_title, command))
            errcode, errmsg = utilities.execute(command)
            if errcode:
                self.info("Failed to tile product {}; error code: {}, error msg:\n{}"
                          .format(product_title, errcode, errmsg))
