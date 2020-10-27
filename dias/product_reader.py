import os
from dias.logger import Logger


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
