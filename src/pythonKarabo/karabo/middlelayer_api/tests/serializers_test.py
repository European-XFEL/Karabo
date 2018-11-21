from lxml import etree
import os
import shutil
from unittest import TestCase, main

from karabo.middlelayer import (
    Configurable, encodeXML, decodeXML, Hash, loadFromFile, VectorDouble,
    saveToFile
)
from karabo.testing.utils import xml_is_equal

TST_WORKING_DIR = "/tmp/serializers_tests"

# The following XML was generated using the C++ API `saveToFile`
BOUND_HASH_XML = """<?xml version="1.0"?>
<root KRB_Artificial="" KRB_Type="HASH">
  <akey KRB_Type="STRING">aval</akey>
  <another KRB_Type="HASH">
    <nested KRB_Type="DOUBLE">1.618000000000000</nested>
  </another>
</root>
"""

# The following XML is what the middlelayer API generates
MDL_HASH_XML = '<root KRB_Artificial=""><akey KRB_Type="STRING" >aval</akey><another KRB_Type="HASH" ><nested KRB_Type="DOUBLE" >1.618</nested></another></root>'  # noqa

# The following is used accross all tests as template Hash
HASH = Hash('akey', 'aval', 'another', Hash('nested', 1.618))
FILENAME = "all/the/folders/hash.xml"


class TestSerializers(TestCase):
    def setUp(self):
        self.curr_dir = os.curdir
        os.mkdir(TST_WORKING_DIR)
        os.chdir(TST_WORKING_DIR)  # Change the working dir, to not pollute

    def tearDown(self):
        os.chdir(self.curr_dir)
        shutil.rmtree(TST_WORKING_DIR)

    def test_saveToFile(self):
        # Test saving in the current directory
        saveToFile(HASH, 'hash.xml')
        self.assertTrue(os.path.exists('./hash.xml'))

        # Test without any sub-directories existing
        saveToFile(HASH, FILENAME)
        self.assertTrue(os.path.exists(FILENAME))

        with open(FILENAME) as fin:
            read_string = fin.read()
        self.assertEqual(read_string, MDL_HASH_XML)

        read_hash = decodeXML(read_string)
        self.assertEqual(read_hash, HASH)

        # Test with sub-directories already existing
        os.remove(FILENAME)
        self.assertFalse(os.path.exists(FILENAME))
        try:
            saveToFile(HASH, FILENAME)
        except Exception as e:
            self.fail("Failed as directory already exists")
        self.assertTrue(os.path.exists(FILENAME))

        # Test with xml file already existing
        self.assertTrue(os.path.exists(FILENAME))
        try:
            saveToFile(HASH, FILENAME)
        except Exception as e:
            self.fail("Failed as file already exists: {}".format(e))

        # Test with non-Hash value
        with self.assertRaises(Exception):
            saveToFile(None, FILENAME)

    def test_loadFromFile(self):
        # Test valid xml
        with open("hash.xml", "w") as fout:
            fout.write(encodeXML(HASH))

        h = loadFromFile("hash.xml")
        self.assertEqual(h, HASH)

        # Test malformatted xml
        with open("bad.xml", "w") as fout:
            fout.write(MDL_HASH_XML[:-5])

        with self.assertRaises(Exception):
            loadFromFile("bad.xml")

    def test_load_bound_hash_xml(self):
        with open("bash.xml", "w") as fout:
            fout.write(BOUND_HASH_XML)

        h = loadFromFile("bash.xml")
        self.assertEqual(h, HASH)

    def test_device_hash(self):
        class A(Configurable):
            vector = VectorDouble(defaultValue=[1.0, 2.0], minSize=1,
                                  maxSize=2)

        a = A()
        schema = a.getClassSchema()
        self.assertEqual(schema.hash['vector', 'minSize'], 1)
        self.assertEqual(schema.hash['vector', 'maxSize'], 2)

        encoded = encodeXML(schema.hash)
        root = etree.fromstring(encoded)
        self.assertEqual(root.tag, 'root')
        for child in root.iter('vector'):
            self.assertEqual(child.attrib['minSize'], 'KRB_UINT32:1')
            self.assertEqual(child.attrib['maxSize'], 'KRB_UINT32:2')

        decoded = decodeXML(encoded)
        self.assertEqual(decoded['vector', 'minSize'], 1)
        self.assertEqual(decoded['vector', 'maxSize'], 2)


if __name__ == "__main__":
    main()
