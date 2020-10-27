
class ProductReader:
    def __init__(self, infile):
        self.products = self.read_products_from_file(infile)

    def get_products(self):
        return self.products

    @staticmethod
    def read_products_from_file(filename):
        try:
            with open(filename) as f:
                lines = f.read().splitlines()
            return lines
        except OSError:
            print("Unable to open file " + filename)
            return []
