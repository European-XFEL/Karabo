import unittest
from karabo.karathon import *
from .configuration_example_classes import *


class  Serializer_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Serializer_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None
    
    def test_textserializer_hash(self):
        try:
            config = Hash("indentation", -1)
            ser = TextSerializerHash.create("Xml", config)     # create default serializer
            hash = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])
            archive = ser.save(hash)
            self.assertEqual(archive,'<?xml version="1.0"?><root KRB_Artificial="" KRB_Type="HASH"><a KRB_Type="HASH"><b KRB_Type="HASH"><c KRB_Type="INT32">1</c></b></a><x KRB_Type="HASH"><y KRB_Type="HASH"><z KRB_Type="VECTOR_INT32">1,2,3,4,5,6,7</z></y></x></root>')
            hash2 = ser.load(archive)
            self.assertEqual(similar(hash, hash2), True)
            
        except Exception as e:
            self.fail("test_textserializer_hash exception group 1: " + str(e))
        
    def test_textserializer_schema_xml(self):
        try:
            config = Hash("indentation", -1)
            ser = TextSerializerSchema.create("Xml", config)     # create default serializer
            schema = Shape.getSchema("EditableCircle")
            archive = ser.save(schema)      # serialize schema
            schema2 = ser.load(archive)
            self.assertEqual(similar(schema.getParameterHash(), schema2.getParameterHash()), True)
            
        except Exception as e:
            self.fail("test_textserializer_schema_xml exception group 1: " + str(e))

    def test_textserializer_schema_xsd(self):
        try:
            config = Hash("indentation", -1)
            ser = TextSerializerSchema.create("Xsd", config)     # create default serializer
            schema = Shape.getSchema("EditableCircle")
            archive = ser.save(schema)      # serialize schema
            self.assertEqual(archive,'<?xml version="1.0"?><xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:a="http://www.karabo.eu"><xs:element name="EditableCircle"><xs:complexType><xs:all><xs:element name="shadowEnabled" type="xs:boolean" default="0" minOccurs="0" maxOccurs="1"><xs:annotation><xs:documentation><a:description>Shadow enabled</a:description><a:displayedName>Shadow</a:displayedName><a:requiredAccessLevel>1</a:requiredAccessLevel><a:default>0</a:default><a:accessType>1</a:accessType></xs:documentation></xs:annotation></xs:element><xs:element name="radius" default="10.000000000000000" minOccurs="0" maxOccurs="1"><xs:annotation><xs:documentation><a:description>The radius of the circle</a:description><a:displayedName>Radius</a:displayedName><a:alias>1</a:alias><a:requiredAccessLevel>1</a:requiredAccessLevel><a:default>10.000000000000000</a:default><a:accessType>4</a:accessType><a:unitName>meter</a:unitName><a:unitSymbol>m</a:unitSymbol><a:metricPrefixName>milli</a:metricPrefixName><a:metricPrefixSymbol>m</a:metricPrefixSymbol></xs:documentation></xs:annotation><xs:simpleType><xs:restriction base="xs:double"><xs:minExclusive value="0.000000000000000" /><xs:maxExclusive value="100.000000000000000" /></xs:restriction></xs:simpleType></xs:element></xs:all></xs:complexType></xs:element></xs:schema>')
            
        except Exception as e:
            self.fail("test_textserializer_schema_xsd exception group 1: " + str(e))

    def test_binaryserializer_hash(self):
        try:
            ser = BinarySerializerHash.create("Bin")
            hash = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7], 'e.f.e', None, 'e.g.h', [None, None, None, None, None])
            hash.setAttribute('e.g.h', 'attr1', None)
            hash.setAttribute('e.f.e', 'attrList', [None, None])
            archive = ser.save(hash)      # serialize hash
            self.assertEqual(archive.__class__.__name__, 'bytes')
            hash2 = ser.load(archive)
            self.assertTrue(similar(hash, hash2))
            
        except Exception as e:
            self.fail("test_binaryserializer_hash exception group 1: " + str(e))
    
    def test_io_write_read_hash(self):
        try:
            config = Hash("filename", "/tmp/test_io_writeread_hash.xml", "format.Xml.indentation", -1)
            out = OutputHash.create("TextFile", config)
            hash = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])
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
            saveToFile(h, "/tmp/MyFile.xml", Hash("format.Xml.indentation", 3))
            
            h2 = Hash()
            loadFromFile(h2, "/tmp/MyFile.xml")
            self.assertEqual(h2['a'], 10)
            self.assertEqual(h2['b'], "Hallo World")
        
        except Exception as e:
            self.fail("test_io_savetofile_loadfromfile exception group 1: " + str(e))   
        
        try:
            h = Hash("a", 10, "b", "Hallo World")
            saveToFile(h, "/tmp/MyFile.xml", Hash("format.Xml.indentation", 1))
            
            h2 = loadFromFile("/tmp/MyFile.xml")
            self.assertEqual(h2['a'], 10)
            self.assertEqual(h2['b'], "Hallo World")
            
        except Exception as e:
            self.fail("test_io_savetofile_loadfromfile exception group 2: " + str(e))    
        

if __name__ == '__main__':
    unittest.main()

