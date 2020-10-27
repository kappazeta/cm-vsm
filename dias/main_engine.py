import argparse

from dias.product_reader import ProductReader
from dias.product_tiler import ProductTiler
from dias.downloader import Downloader
from dias.logger import Logger


class MainEngine(Logger):
    def __init__(self, data_dir):
        super(MainEngine, self).__init__(data_dir)

    def start(self):
        product_reader = ProductReader(self.data_dir)
        downloader = Downloader(product_reader, self.data_dir)
        product_tiler = ProductTiler(product_reader, self.data_dir)

        downloader.start()
        product_tiler.start()


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("-d", "--datadir", action="store", dest="data_dir", type=str,
                   default="~/data/s2_zip",
                   help="Path to folder holding unpacked Sentinel-2 products.")

    args = p.parse_args()
    main_engine = MainEngine(args.data_dir)
    main_engine.start()
