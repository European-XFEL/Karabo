'''
Created on Oct 22, 2012

@author: irinak
'''

import unittest
import filecmp
from libkarabo import *

'''
Free function to write given 'schema' in XSD format into file 'fileName'
'''
def schemaToXsd(schema, fileName):
    inFile = Hash()
    inFile.setFromPath("TextFile.filename", fileName)
    inFile.setFromPath("TextFile.format.Xsd.indentation", 2)
    outWrite = WriterSchema.create(inFile)
    outWrite.write(schema)
    print "\nExpected parameters written into file: ", fileName, "\n"

'''
Decoration function to hide some details on schema creation from the user
'''
def schemamethod(f):

    def deco(cls):
        master = Schema()
        expected = master.initParameterDescription(cls.__name__, AccessType(INIT|READ|WRITE), "")
        f(expected)
        return master

    deco = classmethod(deco)
    return deco

'''
Class for testing simple schema elements of types INT32, INT64, DOUBLE, STRING, BOOL 
'''
class TestClass1(object):
    @schemamethod  
    def expectedParameters(expected):
        e = INT32_ELEMENT(expected)
        e.key("myInt")
        e.displayedName("myIntegerElement")
        e.description("Input first integer element")
        e.alias(10) #alias of integer type 
        e.assignmentOptional().defaultValue(5)
        e.reconfigurable()
        e.commit()
        
        e = UINT32_ELEMENT(expected)
        e.key("myUInt")
        e.alias(5.5) #alias of double type 
        e.displayedName("myIntegerElement")
        e.description("Input first integer element")
        e.assignmentOptional().defaultValue(10)
        e.reconfigurable()
        e.commit()
        
        e = INT64_ELEMENT(expected)
        e.key("myIntLong")
        e.alias("testAlias") #alias of string type
        e.displayedName("myIntegerLongElement")
        e.description("Input second long long element")
        e.assignmentMandatory()
        e.reconfigurable()
        e.commit()
        
        e = DOUBLE_ELEMENT(expected)
        e.key("myDouble")
        e.displayedName("myDoubleElement")
        e.description("Input double element")
        e.assignmentMandatory()
        e.reconfigurable()
        e.commit()

        e = STRING_ELEMENT(expected)
        e.key("myString") 
        e.displayedName("myStringElement")
        e.description("Input string element")
        e.options("test, Hello, World", ",")
        e.assignmentOptional().defaultValue("Hello")
        e.reconfigurable()
        e.commit()

        e = BOOL_ELEMENT(expected)
        e.key("myBool")
        e.displayedName("myBoolElement")
        e.description("Input bool element")
        e.assignmentOptional().defaultValue(True)
        e.reconfigurable()
        e.commit()



'''
Class 'TestClass2' for testing vector schema elements of types 
VECTOR_INT32, VECTOR_INT64, VECTOR_DOUBLE, VECTOR_STRING, VECTOR_BOOL 
'''
class TestClass2(object):   
    @schemamethod	  
    def expectedParameters(expected):
        e = VECTOR_INT32_ELEMENT(expected)
        e.key("myVecInt1")
        e.displayedName("First Integer Vector")
        e.description("First integer vector element")
        e.assignmentOptional().defaultValueFromString('1, 2, 3, 4, 5')
        e.minSize(4).maxSize(10)
        e.minInc(1).maxExc(20)        
        e.reconfigurable()
        e.commit()

        e = VECTOR_INT32_ELEMENT(expected)
        e.key("myVecInt2")
        e.displayedName("Second Integer Vector")
        e.description("Second integer vector element")
        e.assignmentOptional().defaultValue([10, 20, 30])
        e.reconfigurable()
        e.commit()

        e = VECTOR_INT64_ELEMENT(expected)
        e.key("myVecLongInt")
        e.displayedName("Long Vector")
        e.description("Long vector element")
        e.assignmentOptional().defaultValue([5L, 7L, 9L])
        e.reconfigurable()
        e.commit()

        e = VECTOR_DOUBLE_ELEMENT(expected)
        e.key("myVecDouble")
        e.displayedName("Double Vector")
        e.description("Double vector element")
        e.unitName("watt")
        e.unitSymbol("W")
        e.assignmentOptional().defaultValue([1.1, -1.1, 1.111])
        e.reconfigurable()
        e.commit()
 
        e = VECTOR_BOOL_ELEMENT(expected)
        e.key("myBoolVector")
        e.displayedName("Bool Vector")
        e.description("Bool vector element")
        e.assignmentOptional().defaultValue([True, False, True])
        e.reconfigurable()
        e.commit()

        e = VECTOR_STRING_ELEMENT(expected)
        e.key("myVecString")
        e.displayedName("String Vector")
        e.description("String vector description")
        e.assignmentOptional().defaultValue(["hallo", "next", "elem"])
        e.reconfigurable()
        e.commit()
        

'''
Class 'TestClass3' for testing schema elements:
CHOICE_ELEMENT_CONNECTION, CHOICE_ELEMENT_BROKERCONNECTION, 
IMAGE_ELEMENT, INTERNAL_ANY_ELEMENT
as well as 
customer's Element of Complex Type defined by a customer
in the class 'SomeCustomerElement' 
that inherits from the base class 'ComplexElement'.
'''
class SomeCustomerElement(ComplexElement):
    
    def __init__(self, expected):
        self.m_outerElement=ComplexElement(expected)

        self.m_outerElement.reconfigureAndRead()

        self.innerElement=Schema()

        self.m_myInt=INT32_ELEMENT(expected)
        self.m_myInt.key("myNewInt")
        self.m_myInt.displayedName("myNewIntegerElement")
        self.m_myInt.description("Input new integer element")
        self.m_myInt.assignmentOptional().defaultValue(7)
        self.m_myInt.reconfigurable()
        
        self.m_myStr=STRING_ELEMENT(expected)
        self.m_myStr.key("myNewString")
        self.m_myStr.displayedName("myNewStringElement")
        self.m_myStr.description("Input NEW string element")
        self.m_myStr.options("test, Hello, World, new", ",")
        self.m_myStr.assignmentOptional().defaultValue("Hello")
        self.m_myStr.reconfigurable()
    
    def key(self, keyName):
        self.m_outerElement.key(keyName)      
    
    def displayedName(self, displayedName):
        self.m_outerElement.displayedName(displayedName)      
    
    def description(self, desk):
        self.m_outerElement.description(desk)
        
    def assignmentOptional(self):
        self.m_outerElement.assignmentOptional()
    
    def readOnly(self):
        self.m_outerElement.readOnly()
    
    def commit(self): 
        self.m_outerElement.assignmentOptional()
       
        self.innerElement = self.m_outerElement.commit()
        self.m_myInt.commit(self.innerElement)
        self.m_myStr.commit(self.innerElement)
        
class TestClass3(object):   
    
    @schemamethod	  
    def expectedParameters(expected):
        e = CHOICE_ELEMENT_CONNECTION(expected)
        e.key("myConnection")
        e.displayedName("my Element Connection")
        e.description("my Connection description")
        e.assignmentOptional().defaultValue("Snmp")
        e.reconfigurable()
        e.commit()
        
        #element CHOICE_ELEMENT_BROKERCONNECTION works correctly, tested
        #e = CHOICE_ELEMENT_BROKERCONNECTION(expected)
        #e.key("myBrokerConnection")
        #e.displayedName("my Element BrokerConnection")
        #e.description("my Element BrokerConnection description")
        #e.assignmentOptional().defaultValue("Jms")
        #e.reconfigurable()
        #e.commit()
        
        e = IMAGE_ELEMENT(expected)
        e.key("myImage")
        e.displayedName("myImage")
        e.description("myImage description")
        e.commit()
        
        e = INTERNAL_ANY_ELEMENT(expected)
        e.key("anyInternalElemKey")
        e.description("internal any element test")
        e.reconfigurable()
        e.commit()
        
        e = SomeCustomerElement(expected)
        e.key("SomeCustomerElement")
        e.displayedName("someCustomerElement")
        e.description("someCustomerElement description")
        e.assignmentOptional()
        e.readOnly()
        e.commit()
        
        e = OVERWRITE_ELEMENT(expected)
        e.key("overwriteElemKey")
        e.commit()

        e = DOUBLE_TARGETACTUAL_ELEMENT(expected)
        e.key("temperature")
        e.displayedName("Sensor temperature")
        e.unitName("degree celsius")
        e.unitSymbol("deg")
        e.description("Configures the temperature to which the device should be cooled")
        e.targetAssignmentOptional().targetDefaultValue(0)
        e.targetHardMin(-50)
        e.actualWarnHigh(23)
        e.actualAlarmHigh(40)
        e.commit()


class  Schema_TestCase(unittest.TestCase):
    def setUp(self):
        self.resourcesdir = "../../../src/pythonKarabo/tests/resources"

    def test_schema1_(self):
        s1 = TestClass1.expectedParameters()
        print "Expected parameters of TestClass1:\n", s1
        schemaToXsd(s1, "/tmp/TestClass1.xsd")
        
        compare = filecmp.cmp(self.resourcesdir+"/TestClass1_etalon.xsd", "/tmp/TestClass1.xsd")
        self.assertEqual(compare, True, "Schema written into file '/tmp/TestClass1.xsd' differs from etalon file 'TestClass1_etalon.xsd' ")
        
        self.assertTrue(s1.hasKey("myInt"), "Check that schema 's1' has key 'myInt' ")
        self.assertTrue(s1.hasAlias("testAlias"), "Check that schema 's1' has alias 'testAlias' ")
        self.assertTrue(s1.hasAlias(10), "Check that schema 's1' has alias '10' ")
        self.assertTrue(s1.hasAlias(5.5), "Check that schema 's1' has alias '5.5' ")
        self.assertTrue(s1.keyHasAlias("myInt"), "Check that key 'myInt' of the schema 's1' has alias")
        self.assertTrue(s1.keyHasAlias("myUInt"), "Check that key 'myUInt' of the schema 's1' has alias")
        self.assertTrue(s1.keyHasAlias("myIntLong"), "Check that key 'myIntLong' of the schema 's1' has alias")
        self.assertFalse(s1.keyHasAlias("myBool"), "Check that key 'myBool' of the schema 's1' has no alias")
    
    def test_schema2_(self):
        s2 = TestClass2.expectedParameters()
        print "Expected parameters of TestClass2:\n", s2
        schemaToXsd(s2, "/tmp/TestClass2.xsd")
        
        compare = filecmp.cmp(self.resourcesdir+"/TestClass2_etalon.xsd", "/tmp/TestClass2.xsd")
        self.assertEqual(compare, True, "Schema written into file '/tmp/TestClass2.xsd' differs from etalon file 'TestClass2_etalon.xsd' ")
        
        self.assertTrue(s2.hasKey("myVecString"), "Check that schema of 'TestClass2' has key 'myVecString' ")
        print "get help : \n", s2.help("myVecString")

        print "====================================================="
    
    def test_schema3_(self):
        s3 = TestClass3.expectedParameters()
        print "Expected parameters of TestClass3:\n", s3
        schemaToXsd(s3, "/tmp/TestClass3.xsd")

        compare = filecmp.cmp(self.resourcesdir+"/TestClass3_etalon.xsd", "/tmp/TestClass3.xsd")
        self.assertEqual(compare, True, "Schema written into file '/tmp/TestClass3.xsd' differs from etalon file 'TestClass3_etalon.xsd' ")
        self.assertTrue(s3.hasKey("myImage"), "Check that schema of 'TestClass3' has key 'myImage' ")
        self.assertTrue(s3.hasKey("anyInternalElemKey"), "Check that schema of 'TestClass3' has key 'anyInternalElemKey' ")

if __name__ == '__main__':
    unittest.main()

