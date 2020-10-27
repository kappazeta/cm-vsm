import os
import shutil
from pathlib import Path
from multiprocessing import Process, Queue

import dias.s3 as s3
import dias.utilities as utilities


class Downloader:
    def __init__(self, product_reader, outdir):
        self.n_threads = 2
        self.product_reader = product_reader
        self.outdir = outdir
        self.s3_bucket_name = "EODATA"
        self.disk_quota_str = "200G"
        self.disk_quota = utilities.size_from_str(self.disk_quota_str)

    def start(self):
        product_titles = self.product_reader.get_products()
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
            product_path = s3.get_path(product_title)
            download_size = s3.get_size(self.s3_bucket_name, product_path)
            dir_size = utilities.get_dir_size(self.outdir) + download_size
            if dir_size > self.disk_quota:
                print("Skipping product {} as disk is full: {}/{}"
                      .format(product_title, utilities.size_to_str(dir_size), self.disk_quota_str))
                continue

            download_path = os.path.join(self.outdir, product_title + ".SAFE.part")
            if not Path(download_path).is_dir():
                os.makedirs(download_path)

            try:
                command, suppress = s3.get_command(self.s3_bucket_name, product_path, download_path)
                print("Downloading product {} as {}")
                self.download_product(command + suppress, download_size, download_path, product_title)
            except RuntimeError as error:
                print(str(error))

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
