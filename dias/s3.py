import re
import subprocess


def get_path(product_title):
    """
    Generate S3 path from product title
    """

    # Sample title: 'S2A_MSIL2A_20200506T093041_N0214_R136_T35VMF_20200506T140446'
    split_title = product_title.split('_')

    satellite = "Sentinel-" + split_title[0][1]
    product_type1 = split_title[1][0:3]
    product_type2 = split_title[1][3:]
    year = split_title[2][0:4]
    month = split_title[2][4:6]
    day = split_title[2][6:8]

    # Sample prefix: 'Sentinel-2/MSI/L2A/2020/05/06/'
    prefix = "{}/{}/{}/{}/{}/{}/".format(satellite, product_type1, product_type2, year, month, day)

    # Sample path: 'Sentinel-2/MSI/L2A/2020/05/06/S2A_MSIL2A_20200506T093041_N0214_R136_T35VMF_20200506T140446.SAFE/'
    return prefix + product_title + ".SAFE/"


def get_command(bucket_name, product_path, output_path):
    """
    Generate a S3 command to download the product.
    """
    # It might be, that in quiet mode the download misbehaves. If this happens, switch to verbose mode
    quiet_flag = "--quiet "
    command = "s3cmd get --recursive {}--force s3://{}/{} {}".format(quiet_flag, bucket_name, product_path, output_path)
    return command


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
