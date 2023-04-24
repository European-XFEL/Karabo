# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import subprocess
import unittest

import karabo.config_db


def get_python_files():
    module = karabo.config_db.__file__
    dirlist = os.listdir(os.path.dirname(module))
    return dirlist


@unittest.skip(reason="New flake8 version")
def test_code_quality_flake8():
    file_list = get_python_files()
    command = ['flake8', *[os.path.abspath(py_file) for py_file in file_list]]
    subprocess.check_call(command)
