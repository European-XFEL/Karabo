import unittest
from karabo.karathon import *
from configuration_example_classes import *


class  Io_TestCase(unittest.TestCase):
    def setUp(self):
        self.schema = Schema()
        TestStruct1.expectedParameters(self.schema)
    

    def tearDown(self):
        del self.schema
        self.schema = None

    def test_io1_write_xml_schema(self):
        try:
            out = OutputSchema.create("TextFile", Hash("filename", "/tmp/testschema.xml", "format.Xml.indentation", 3))
            self.assertFalse(out == None)
            out.write(self.schema)
            del out
            
            saveToFile(self.schema, "/tmp/testschema2.xml", Hash("format.Xml.indentation", 3))
        except Exception as e:
            self.fail("test_io_write_xml_schema exception group: " + str(e))

    def test_io2_read_xml_schema(self):
        try:
            inp = InputSchema.create("TextFile", Hash("filename", "/tmp/testschema.xml"))
            self.assertFalse(inp == None)
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
            out = OutputSchema.create("BinaryFile", Hash("filename", "/tmp/testschema.bin"))
            self.assertFalse(out == None)
            out.write(self.schema)
            del out
            
            saveToFile(self.schema, "/tmp/testschema2.bin")
        except Exception as e:
            self.fail("test_io_write_binary_schema exception group: " + str(e))

    def test_io2_read_binary_schema(self):
        try:
            inp = InputSchema.create("BinaryFile", Hash("filename", "/tmp/testschema.bin"))
            self.assertFalse(inp == None)
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

