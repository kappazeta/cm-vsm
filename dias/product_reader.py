# Sentinel-2 product reader
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
from logger import Logger


class ProductReader(Logger):
    def __init__(self, data_dir):
        super(ProductReader, self).__init__(data_dir)
        infile = os.path.join(data_dir, "products.dat")
        self.products = self.read_products_from_file(infile)

    def get_products(self):
        return self.products

    def read_products_from_file(self, filename):
        try:
            with open(filename) as f:
                lines = f.read().splitlines()
            return lines
        except OSError:
            self.info("Unable to open file " + filename)
            return []
