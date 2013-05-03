# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from libkarathon import *
from configuration_test_classes import *


class  Serializer_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Serializer_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None
    '''
    def test_serializer_hashxml(self):
        try:
            ser = HashXmlSerializer()     # create default serializer
            hash = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])
            archive = ser.save(hash)
            self.assertEqual(archive,'<?xml version="1.0"?><root KRB_Artificial="" KRB_Type="HASH"><a KRB_Type="HASH"><b KRB_Type="HASH"><c KRB_Type="INT32">1</c></b></a><x KRB_Type="HASH"><y KRB_Type="HASH"><z KRB_Type="VECTOR_INT32">1,2,3,4,5,6,7</z></y></x></root>')
            hash2 = ser.load(archive)
            self.assertEqual(similar(hash, hash2), True)
            
        except Exception,e:
            self.fail("test_serializer_hashxml exception group 1: " + str(e))
        
    def test_serializer_schemaxml(self):
        try:
            ser = SchemaXmlSerializer()     # create default serializer
            schema = Shape.getSchema("EditableCircle")
            archive = ser.save(schema)      # serialize schema
            #self.assertEqual(archive,'<?xml version="1.0"?><root KRB_Artificial="" KRB_Type="HASH"><a KRB_Type="HASH"><b KRB_Type="HASH"><c KRB_Type="INT32">1</c></b></a><x KRB_Type="HASH"><y KRB_Type="HASH"><z KRB_Type="VECTOR_INT32">1,2,3,4,5,6,7</z></y></x></root>')
            schema2 = ser.load(archive)
            self.assertEqual(similar(schema.getParameterHash(), schema2.getParameterHash()), True)
            
        except Exception,e:
            self.fail("test_serializer_schemaxml exception group 1: " + str(e))

    def test_serializer_schemaxsd(self):
        try:
            ser = SchemaXsdSerializer()     # create default serializer
            schema = Shape.getSchema("EditableCircle")
            archive = ser.save(schema)      # serialize schema
            self.assertEqual(archive,'<?xml version="1.0"?><xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:a="http://www.karabo.eu"><xs:element name="EditableCircle"><xs:complexType><xs:all><xs:element name="shadowEnabled" type="xs:boolean" default="0" minOccurs="0" maxOccurs="1"><xs:annotation><xs:documentation><a:description>Shadow enabled</a:description><a:displayedName>Shadow</a:displayedName><a:default>0</a:default><a:accessType>1</a:accessType></xs:documentation></xs:annotation></xs:element><xs:element name="radius" default="10.000000000000000" minOccurs="0" maxOccurs="1"><xs:annotation><xs:documentation><a:description>The radius of the circle</a:description><a:displayedName>Radius</a:displayedName><a:alias>1</a:alias><a:default>10.000000000000000</a:default><a:accessType>4</a:accessType><a:unitName>meter</a:unitName><a:unitSymbol>m</a:unitSymbol><a:metricPrefixName>milli</a:metricPrefixName><a:metricPrefixSymbol>m</a:metricPrefixSymbol></xs:documentation></xs:annotation><xs:simpleType><xs:restriction base="xs:double"><xs:minExclusive value="0.000000000000000" /><xs:maxExclusive value="100.000000000000000" /></xs:restriction></xs:simpleType></xs:element></xs:all></xs:complexType></xs:element></xs:schema>')
            
        except Exception,e:
            self.fail("test_serializer_schemaxsd exception group 1: " + str(e))

    def test_serializer_hashbinary(self):
        try:
            ser = HashBinarySerializer()
            hash = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])
            archive = ser.save(hash)      # serialize hash
            self.assertEqual(archive.__class__.__name__, 'bytearray')
            hash2 = ser.load(archive)
            self.assertEqual(similar(hash, hash2), True)
            
        except Exception,e:
            self.fail("test_serializer_schemaxsd exception group 1: " + str(e))
    '''
        
if __name__ == '__main__':
    unittest.main()

