# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import os
import subprocess
import unittest

import karabo.middlelayer_devices


def get_python_files():
    module = karabo.middlelayer_devices.__file__
    dirlist = os.listdir(os.path.dirname(module))
    return dirlist


@unittest.skip(reason="New flake8 version")
def test_code_quality_flake8():
    file_list = get_python_files()
    command = ['flake8', *[os.path.abspath(py_file) for py_file in file_list]]
    subprocess.check_call(command)
