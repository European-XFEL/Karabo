#!/usr/bin/python

from libpyexfel import *

class VectorTests(ModulePy):

  def compute(self):
    '''compute method'''
    print "VectorTests. compute method"
    print "print a: ", self.a

  def expectedParameters(cls,expected):
    '''define parameters needed for this module '''

    print "VectorTests. expectedParameters - in python"

    e = INT32_ELEMENT(expected)
    e.key("a")
    e.displayedName("A")
    e.description("first element")
    e.advanced()
    e.assignmentMandatory()
    e.commit()

    e = VECTOR_INT32_ELEMENT(expected)
    e.key("vInt32")
    e.description("vector of integer numbers")
    e.displayedName("Int32 Vector")
    e.minSize(2)
    e.maxSize(10)
    e.minInc(5)
    e.maxInc(200)
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = VECTOR_STRING_ELEMENT(expected)
    e.key("VectorString")
    e.displayedName("Vector of string")
    e.description("Description: Vector of string")
    e.assignmentOptional().noDefaultValue()
    e.commit();

    e = VECTOR_BOOL_ELEMENT(expected)
    e.key("vectorBool")
    e.description("vector of boolean")
    e.displayedName("vectorOfBoolValues")
    e.minSize(2)
    e.maxSize(10)
    e.assignmentOptional().noDefaultValue()
    e.commit();
    
    e = VECTOR_DOUBLE_ELEMENT(expected)
    e.key("vectorDouble")
    e.description("vector of double")
    e.displayedName("vectorOfDoubleValues")
    e.minSize(2)
    e.maxSize(10)
    e.minInc(-8.9)
    e.maxInc(15.111)
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = VECTOR_INT8_ELEMENT(expected)
    e.key("vectorInt8")
    e.description("vector of INT8 (signed char)")
    e.displayedName("vectorOfINT8Values")
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = VECTOR_UINT8_ELEMENT(expected)
    e.key("vectorUInt8")
    e.description("vector of unsigned char")
    e.displayedName("vectorOfUINT8Values")
    e.minSize(2)
    e.maxSize(10)
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = VECTOR_INT16_ELEMENT(expected)
    e.key("vectorInt16")
    e.description("vector of INT16 (signed short)")
    e.displayedName("vectorOfINT16Values")
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = VECTOR_UINT16_ELEMENT(expected)
    e.key("vectorUInt16")
    e.description("vector of unsigned short")
    e.displayedName("vectorOfUINT16Values")
    e.minSize(2)
    e.maxSize(10)
    e.assignmentOptional().noDefaultValue()
    e.commit()

    e = BOOL_ELEMENT(expected)
    e.key("boolean")
    e.displayedName("boolean")
    e.description("Test boolean value")
    e.assignmentOptional().defaultValue(True)
    e.commit()

    e = STRING_ELEMENT(expected)
    e.key("String")
    e.displayedName("string value")
    e.description("Description: string")
    e.assignmentOptional().noDefaultValue()
    e.commit();

    e = FLOAT_ELEMENT(expected)
    e.key("FloatElement")
    e.displayedName("Float")
    e.description("Float data type")
    e.minInc(0.0).maxInc(20.0)
    e.assignmentOptional().defaultValue(0.0)
    e.reconfigurable()
    e.commit();

    e = PATH_ELEMENT(expected)
    e.key("PathToDir")
    e.displayedName("Path to Directory")
    e.description("Directory to search")
    e.assignmentOptional().defaultValueFromString("a/b/dir")
    e.advanced()
    e.commit();

    e = VECTOR_PATH_ELEMENT(expected)
    e.key("vectorPATH")
    e.description("vector of PATHs")
    e.displayedName("vectorPATH")
    e.minSize(2)
    e.maxSize(10)
    e.assignmentOptional().noDefaultValue()
    e.commit()

  def configure(self, conf):
    ''' configure the object from config'''
    self.a = conf.getFromPath("a")

  def create(cls):
    ''' create VectorTests object
        Needed for C++
    '''
    return VectorTests()

  expectedParameters = classmethod(expectedParameters)
  create = classmethod(create)
