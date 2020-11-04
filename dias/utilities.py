# Generic utility functions
#
# Copyright 2020 KappaZeta Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import re
from pathlib import Path
import subprocess


SIZE_POWERS = ["B", "K", "M", "G", "T", "P"]


def get_dir_size(path):
    """
    Recursively get the size of a directory, in bytes.
    @param path: Path to the directory
    @return: Directory size in bytes.
    """
    # https://stackoverflow.com/a/1392549/1692112
    return sum(f.stat().st_size for f in Path(path).glob('**/*') if f.is_file())


def size_from_str(text):
    """
    Get number of bytes from a string similar to "5G", "3 KiB", etc.
    @param text: Text to extract the size from.
    @return: Number of bytes.
    """
    m = re.match(r"([\d.]+)\s*([ikKBMGTP])", text)
    if m:
        value = float(m.group(1))
        unit = m.group(2).capitalize()

        value *= 1024**SIZE_POWERS.index(unit)

        return value


def size_to_str(size):
    """
    Convert number of bytes into a string similar to "5.2 KiB"
    @param size: Number of bytes.
    @return: Size with a unit, as a string.
    """
    for i in range(len(SIZE_POWERS) - 1, 0, -1):
        if size >= 1024**i:
            return "{:.2f} {}iB".format(size / (1024**i), SIZE_POWERS[i])
    return "{} B".format(size)


def execute(command):
    """
    Execute a command.
    @param command: Command to execute.
    @return: True if the command was executed successfully, otherwise False.
    """
    process = subprocess.Popen(command, shell=True, stderr=subprocess.PIPE)
    process.wait()
    _, stderr = process.communicate()

    return [process.returncode, stderr]
