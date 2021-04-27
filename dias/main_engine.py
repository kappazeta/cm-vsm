# Tool for downloading and splitting specific Sentinel-2 products in DIAS
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

import argparse

from product_reader import ProductReader
from product_tiler import ProductTiler
from downloader import Downloader
from uploader import Uploader
from logger import Logger


class MainEngine(Logger):
    def __init__(self, data_dir):
        super(MainEngine, self).__init__(data_dir)

    def start(self):
        product_reader = ProductReader(self.data_dir)
        downloader = Downloader(product_reader, self.data_dir)
        product_tiler = ProductTiler(product_reader, self.data_dir)
        uploader = Uploader(product_reader, self.data_dir)

        downloader.start()
        # product_tiler.start()
        # uploader.start()


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("-d", "--datadir", action="store", dest="data_dir", type=str,
                   default="~/data/s2_zip",
                   help="Path to folder holding unpacked Sentinel-2 products.")

    args = p.parse_args()
    main_engine = MainEngine(args.data_dir)
    main_engine.start()
