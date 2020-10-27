import argparse
import os

from dias.product_reader import ProductReader
from dias.product_tiler import ProductTiler
from dias.downloader import Downloader


def main(data_dir, infile):
    product_reader = ProductReader(os.path.join(data_dir, infile))
    downloader = Downloader(product_reader, data_dir)
    product_tiler = ProductTiler(product_reader, data_dir)

    downloader.start()
    product_tiler.start()


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("-d", "--datadir", action="store", dest="data_dir", type=str,
                   default="~/data/s2_zip",
                   help="Path to folder holding unpacked Sentinel-2 products.")
    p.add_argument("-f", "--infile", action="store", dest="infile", type=str,
                   default="products.dat",
                   help="File holding product titles that need to be processed.")
    args = p.parse_args()
    main(args.data_dir, args.infile)
