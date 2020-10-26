
import s3
import os
from pathlib import Path
from multiprocessing import Process, Queue


class Downloader:
    def __init__(self):
        self.n_threads = 2
        self.infile = "in/in.dat"
        self.outdir = "out"
        self.s3_bucket_name = "EODATA"

    @staticmethod
    def get_product_titles(filename):
        try:
            with open(filename) as f:
                lines = f.read().splitlines()
            return lines
        except OSError:
            print("Unable to open file " + filename)
            return []

    def start(self):
        product_titles = self.get_product_titles(self.infile)
        n_products = len(product_titles)
        if n_products == 0:
            print('Nothing to download.')
            return

        print("Downloading {} products(s) with {} processes(s)".format(n_products, self.n_threads))

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

        print("Finished downloading.")

    def download(self, control_queue, jobs_queue):
        # If the output directory doesn't exist yet, then create it.
        if not Path(self.outdir).is_dir():
            os.makedirs(self.outdir)

        # In case of ctrl+c, control_queue will become empty
        while (not control_queue.empty()) and (not jobs_queue.empty()):
            product_title = jobs_queue.get()
