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
    BinarySerializerHash, Hash, InputHash, OutputHash, TextSerializerHash,
    TextSerializerSchema, loadFromFile, saveToFile, similar)

from .configuration_example_classes import Shape


class Serializer_TestCase(unittest.TestCase):
    def test_textserializer_hash(self):
        try:
            config = Hash("indentation", -1)
            # create default serializer
            ser = TextSerializerHash.create("Xml", config)
            hash = Hash('a.b.c', 1, 'x.y.z', [1, 2, 3, 4, 5, 6, 7])
            archive = ser.save(hash)
            self.assertEqual(archive,
                             '<?xml version="1.0"?><root KRB_Artificial="" KRB_Type="HASH"><a KRB_Type="HASH"><b KRB_Type="HASH"><c KRB_Type="INT32">1</c></b></a><x KRB_Type="HASH"><y KRB_Type="HASH"><z KRB_Type="VECTOR_INT32">1,2,3,4,5,6,7</z></y></x></root>')  # noqa
            hash2 = ser.load(archive)
            self.assertEqual(similar(hash, hash2), True)

        except Exception as e:
            self.fail("test_textserializer_hash exception group 1: " + str(e))

    def test_textserializer_schema_xml(self):
        try:
            config = Hash("indentation", -1)
            ser = TextSerializerSchema.create("Xml", config)
            schema = Shape.getSchema("EditableCircle")
            archive = ser.save(schema)
            schema2 = ser.load(archive)
            self.assertEqual(similar(schema.getParameterHash(),
                                     schema2.getParameterHash()), True)

        except Exception as e:
            self.fail("test_textserializer_schema_xml exception group 1: "
                      + str(e))

    def test_binaryserializer_hash(self):
        try:
            ser = BinarySerializerHash.create("Bin")
            hash = Hash('a.b.c', 1, 'x.y.z', [1, 2, 3, 4, 5, 6, 7],
                        'e.f.e', None, 'e.g.h', [None, None, None, None, None])
            hash.setAttribute('e.g.h', 'attr1', None)
            hash.setAttribute('e.f.e', 'attrList', [None, None])
            archive = ser.save(hash)  # serialize hash
            self.assertEqual(archive.__class__.__name__, 'bytes')
            hash2 = ser.load(archive)
            self.assertTrue(similar(hash, hash2))

        except Exception as e:
            self.fail("test_binaryserializer_hash exception group 1: "
                      + str(e))

    def test_io_write_read_hash(self):
        try:
            config = Hash("filename", "/tmp/test_io_writeread_hash.xml",
                          "format", "Xml", "Xml.indentation", -1)
            out = OutputHash.create("TextFile", config)
            hash = Hash('a.b.c', 1, 'x.y.z', [1, 2, 3, 4, 5, 6, 7])
            out.write(hash)

            hash2 = Hash()
            config = Hash("filename", "/tmp/test_io_writeread_hash.xml")
            input = InputHash.create("TextFile", config)
            input.read(hash2)
            self.assertTrue(similar(hash, hash2))

        except Exception as e:
            self.fail("test_io_saveload_hash exception group 1: " + str(e))

    def test_io_savetofile_loadfromfile(self):
        try:
            h = Hash("a", 10, "b", "Hallo World")
            saveToFile(h, "/tmp/MyFile.xml", Hash("format", "Xml",
                                                  "Xml.indentation", 3))

            h2 = Hash()
            loadFromFile(h2, "/tmp/MyFile.xml")
            self.assertEqual(h2['a'], 10)
            self.assertEqual(h2['b'], "Hallo World")

        except Exception as e:
            self.fail("test_io_savetofile_loadfromfile exception group 1: "
                      + str(e))

        try:
            h = Hash("a", 10, "b", "Hallo World")
            saveToFile(h, "/tmp/MyFile.xml", Hash("format", "Xml",
                                                  "Xml.indentation", 1))

            h2 = loadFromFile("/tmp/MyFile.xml")
            self.assertEqual(h2['a'], 10)
            self.assertEqual(h2['b'], "Hallo World")

        except Exception as e:
            self.fail("test_io_savetofile_loadfromfile exception group 2: "
                      + str(e))


if __name__ == '__main__':
    unittest.main()
