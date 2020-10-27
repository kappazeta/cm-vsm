import re
import subprocess


def get_path(product_title):
    """
    Generate S3 path from product title
    """

    # Sample title: 'S1A_IW_SLC__1SDV_20180504T044207_20180504T044237_021750_025872_81A4'
    split_title = product_title.split('_')

    satellite = "Sentinel-" + split_title[0][1]
    product_type = split_title[2]
    year = split_title[5][0:4]
    month = split_title[5][4:6]
    day = split_title[5][6:8]

    # Sample prefix: 'Sentinel-1/SAR/SLC/2018/05/04/'
    prefix = "{}/{}/{}/{}/{}/{}/".format(satellite, "SAR", product_type, year, month, day)

    # Sample path: 'Sentinel-1/SAR/SLC/2018/05/04/S1A_IW_SLC__1SDV_20180504T044207_20180504T044237_021750_025872_81A4.SAFE/'
    return prefix + product_title + ".SAFE/"


def get_command(bucket_name, product_path, output_path):
    """
    Generate a S3 command to download the product.
    """
    quiet_flag = "--quiet "

    # Prevent s3cmd printing erroneous errors and warnings by first redirecting stderr to stdout (2>&1)
    # and then sed-ing away error & warning messages.
    # Sample messages:
    # WARNING: Empty object name on S3 found, ignoring.
    # ERROR: Skipping /home/am_app/data/s1_zip/S1A_IW_SLC__1SDV_20180514T161303_20180514T161330_021903_025D6A_CDE5.SAFE/annotation/calibration/: Is a directory
    suppress = ' 2>&1 | sed -u -e "/^WARNING: Empty.\+/d" -e "/^ERROR: Skipping.\+: Is a directory$/d"'
    command = "s3cmd get --recursive {}--force s3://{}/{} {}".format(quiet_flag, bucket_name, product_path, output_path)
    return [command, suppress]


def get_size(bucket_name, product_path, host=""):
    """
    Measure the size of file or folder in bytes from S3 query.
    If remote host is specified, the query will be performed over ssh.
    """
    command = "s3cmd du s3://{}/{}".format(bucket_name, product_path)

    # Sample host: am_app@ts.kappazeta.ee
    if host != "":
        command = 'ssh -o "StrictHostKeyChecking=no" {host} "{cmd}"'.format(host=host, cmd=command)

    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    process.wait()
    stdout, _ = process.communicate()

    if process.returncode != 0:
        return 0

    # Take care of leading and trailing characters
    m = re.match(r"\s*(\d+)\s.*", stdout.decode("utf-8"))
    if m:
        return int(m.group(1))

    return None
