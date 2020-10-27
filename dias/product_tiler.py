import os
from multiprocessing import Process, Queue

import dias.utilities as utilities


class ProductTiler:
    def __init__(self, product_reader, indir):
        self.n_threads = 10
        self.product_reader = product_reader
        self.indir = indir
        self.cvat_exec = "../vsm/build/bin/cvat_vsm -d "

    def start(self):
        product_titles = self.product_reader.get_products()
        n_products = len(product_titles)
        if n_products == 0:
            print('Nothing to split.')
            return

        print("Splitting {} products(s) with {} processes(s)".format(n_products, self.n_threads))

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

        print("Finished splitting.")

    def split(self, control_queue, jobs_queue):
        # In case of ctrl+c, control_queue will become empty
        while (not control_queue.empty()) and (not jobs_queue.empty()):
            product_title = jobs_queue.get()
            product_path = os.path.join(self.indir, product_title + ".SAFE")
            utilities.execute(self.cvat_exec + product_path)
