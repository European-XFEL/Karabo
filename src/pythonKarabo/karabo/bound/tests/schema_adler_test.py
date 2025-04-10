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
import os.path
import subprocess
import sys
import unittest
from zlib import adler32


class Tests(unittest.TestCase):
    def setUp(self):
        self.__starting_dir = os.curdir
        os.chdir(os.path.dirname(os.path.abspath(__file__)))

    def tearDown(self):
        os.chdir(self.__starting_dir)

    def test_adler(self):
        s = b'I love python, Hello world'
        t = adler32(s)
        self.assertEqual(t, 2073102698,
                         "The adler32's behavior changed.")

    def test_schema(self):
        process = subprocess.Popen(
            [sys.executable, "-m", "karabo.bound.launcher",
             "schemaOverwriteVersion",  # fixed default karabo version
             "karabo.bound_device_test", "TestDevice"],
            stdout=subprocess.PIPE)
        schema = process.communicate()[0]
        print(f"\nCross schema test: schema length is {len(schema)}")
        self.assertEqual(adler32(schema), 1645897357,
                         "The generated schema changed. If this is "
                         "desired, change the checksum in the code.")
