# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
            [sys.executable, "-m", "karabo.bound_api.launcher",
             "schemaOverwriteVersion",  # fixed default karabo version
             "karabo.bound_device_test", "TestDevice"],
            stdout=subprocess.PIPE)
        schema = process.communicate()[0]
        print("\nCross schema test: schema length is {}".format(len(schema)))
        self.assertEqual(adler32(schema), 3186809005,
                         "The generated schema changed. If this is "
                         "desired, change the checksum in the code.")
