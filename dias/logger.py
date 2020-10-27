import logging
import os
from string import Template

logger = {}

MAX_NAME_WIDTH = 18
GENERAL_MSG = Template('$src >>> $msg')


class Logger(object):
    def __init__(self, data_dir):
        self.data_dir = data_dir
        self.log = self.get_instance()

    def get_instance(self):
        if 'instance' not in logger.keys():
            logger['instance'] = self.initialize_logger()
        return logger['instance']

    def info(self, msg):
        self.log.info(GENERAL_MSG.substitute(src=self.__class__.__name__.ljust(MAX_NAME_WIDTH), msg=msg))

    def initialize_logger(self):
        log_instance = logging.getLogger()
        log_instance.setLevel(logging.DEBUG)
        formatter = logging.Formatter("%(asctime)s\t%(threadName)s\t%(message)s", datefmt="%Y-%m-%d %H:%M:%S")

        handler = logging.StreamHandler()
        handler.setLevel(logging.INFO)
        handler.setFormatter(formatter)
        log_instance.addHandler(handler)

        filename = "info.log"
        logfile_path = os.path.join(self.data_dir, filename)

        handler = logging.FileHandler(logfile_path, "w", encoding=None, delay="true")
        handler.setLevel(logging.INFO)
        handler.setFormatter(formatter)
        log_instance.addHandler(handler)

        return log_instance


def retrieve_instance():
    return logger['instance']
