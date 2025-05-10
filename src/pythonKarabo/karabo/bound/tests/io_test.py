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
import unittest

from karabo.bound import (
    Hash, InputSchema, OutputSchema, Schema, loadFromFile, saveToFile, similar)

from .configuration_example_classes import TestStruct1


class Io_TestCase(unittest.TestCase):
    def setUp(self):
        self.schema = Schema()
        TestStruct1.expectedParameters(self.schema)

    def tearDown(self):
        del self.schema
        self.schema = None

    def test_io1_write_xml_schema(self):
        try:
            out = OutputSchema.create("TextFile",
                                      Hash("filename", "/tmp/testschema.xml",
                                           "format", "Xml",
                                           "Xml.indentation", 3))
            self.assertIsNotNone(out)
            out.write(self.schema)
            del out

            saveToFile(self.schema, "/tmp/testschema2.xml",
                       Hash("format", "Xml", "Xml.indentation", 3))
        except Exception as e:
            self.fail("test_io_write_xml_schema exception group: " + str(e))

    def test_io2_read_xml_schema(self):
        try:
            inp = InputSchema.create("TextFile",
                                     Hash("filename", "/tmp/testschema.xml"))
            self.assertIsNotNone(inp)
            schema1 = Schema()
            inp.read(schema1)
            del inp

            schema2 = Schema()
            loadFromFile(schema2, "/tmp/testschema2.xml")

            self.assertTrue(similar(schema1, self.schema))
            self.assertTrue(similar(schema2, self.schema))
        except Exception as e:
            self.fail("test_io_read_xml_schema exception group: " + str(e))

    def test_io1_write_binary_schema(self):
        try:
            out = OutputSchema.create("BinaryFile",
                                      Hash("filename", "/tmp/testschema.bin"))
            self.assertIsNotNone(out)
            out.write(self.schema)
            del out

            saveToFile(self.schema, "/tmp/testschema2.bin")
        except Exception as e:
            self.fail("test_io_write_binary_schema exception group: " + str(e))

    def test_io2_read_binary_schema(self):
        try:
            inp = InputSchema.create("BinaryFile",
                                     Hash("filename", "/tmp/testschema.bin"))
            self.assertIsNotNone(inp)
            schema1 = Schema()
            inp.read(schema1)
            del inp

            schema2 = Schema()
            loadFromFile(schema2, "/tmp/testschema2.bin")

            self.assertTrue(similar(schema1, self.schema))
            self.assertTrue(similar(schema2, self.schema))
        except Exception as e:
            self.fail("test_io_read_binary_schema exception group: " + str(e))


if __name__ == '__main__':
    unittest.main()
