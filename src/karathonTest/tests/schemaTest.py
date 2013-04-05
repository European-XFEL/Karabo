'''
Created on March 20, 2013

@author: irinak
'''

from libkarathon import *

'''
Free function to write given 'schema' in XSD format into file 'fileName'
'''
def schemaToXsd(schema, fileName):
    inFile = Hash()
    inFile.set("filename", fileName)
    inFile.set("format.Xsd.indentation", 2)
    outWrite = WriterSchema.create("TextFile", inFile)
    outWrite.write(schema)
    print "\nExpected parameters written into file: ", fileName, "\n"


'''
Decoration function to hide some details on schema creation from the user
'''
def schemamethod(f):

    def deco(cls):
        master = Schema(cls.__name__, AssemblyRules(AccessType(READ | WRITE | INIT)))
        f(master)
        return master

    deco = classmethod(deco)
    return deco


'''
Class for testing simple schema elements of types INT32, INT64, DOUBLE, STRING, BOOL,
and NODE_ELEMENT, LIST_ELEMENT, CHOICE_ELEMENT
'''
class TestClass1(object):
    @schemamethod
    def expectedParameters(expected):
        e = INT32_ELEMENT(expected)
        e.key("myInt")
        e.displayedName("myIntegerElement")
        e.description("Input first integer element")
        e.assignmentOptional().defaultValue(5)
        e.reconfigurable()
        e.commit()
	
	e = INT32_ELEMENT(expected)
        e.key("myIntNew")
        e.displayedName("myIntegerElementNew")
        e.description("Input first integer element new")
	e.readOnly()
        e.commit()
	
	e = NODE_ELEMENT(expected)
	e.key("MyNodeElement")
        e.tags("myNode")
        e.displayedName("MyNodeElem")
        e.description("Description of my node elem")
        e.commit()

        e = DOUBLE_ELEMENT(expected)
	e.key("MyNodeElement.a")
        e.tags("myNode")
        e.displayedName("MyNodeElem_A")
        e.description("Description of my node elem A")
        e.assignmentMandatory()
        e.commit()

        e = LIST_ELEMENT(expected)
	e.key("shapes")
        e.alias(10)
	e.displayedName("shapesAsList")
        e.description("Description of List-element shapes")
        e.min(2)
        e.max(5)
	e.assignmentMandatory()
        #e.assignmentOptional().defaultValue("circle, rectangle")
        e.commit()
	
        #or use CHOICE  instead of LIST above :
	#e = CHOICE_ELEMENT(expected)
	#e.key("shapes")
        #e.displayedName("shapesAsChoice")
	#e.alias("alias")
        #e.description("Description of Choice-element shapes")
        #e.assignmentOptional().defaultValue("circle")
        #e.commit()
	
	e = NODE_ELEMENT(expected)
	e.key("shapes.circle")
        e.tags("shape")
        e.displayedName("Circle")
        e.description("Description of circle")
        e.commit()

        e = INT32_ELEMENT(expected)
	e.key("shapes.circle.radius")
        e.tags("shape")
        e.displayedName("radius")
        e.description("Radius of circle")
        e.minInc(5)
        e.maxExc(10)
        e.assignmentOptional().defaultValue(5)
        e.commit()
	
	e = NODE_ELEMENT(expected).key("shapes.rectangle")
        e.tags("shape")
        e.displayedName("rectangle")
        e.description("Description of rectangle")
        e.commit()

        e = DOUBLE_ELEMENT(expected).key("shapes.rectangle.square")
        e.tags("shape")
        e.displayedName("square")
        e.description("Description of square of rectangle")
        e.assignmentOptional().noDefaultValue()
        e.commit()
	
'''
Class for testing simple schema elements of types INT32, INT64, DOUBLE, STRING, BOOL
as well as VECTOR_ - Elements
'''
class TestClass2(object):
    @classmethod
    def expectedParameters(self, expected):
        e = INT32_ELEMENT(expected)
        e.key("myInt")
	e.alias(10)
        e.displayedName("myIntegerElement")
        e.description("Input first integer element")
        e.assignmentOptional().noDefaultValue()
        e.reconfigurable()
        e.commit()
	
	e = INT32_ELEMENT(expected)
        e.key("myIntNew")
	e.alias("aliasString")
        e.displayedName("myIntegerElementNew")
        e.description("Input first integer element new")
	e.readOnly().alarmHigh(7).alarmLow(2).warnHigh(10).warnLow(2).initialValueFromString("10")
        e.commit()
	
	e = STRING_ELEMENT(expected)
        e.key("myString")
	e.alias(20)
        e.displayedName("myStringElement")
        e.description("Input string element")
	e.options("green, blue")
        e.assignmentOptional().defaultValue("green")
        e.reconfigurable()
        e.commit()
	
	e = BOOL_ELEMENT(expected)
        e.key("myBool")
	e.alias("boolAlias")
        e.displayedName("myBoolgElement")
        e.description("Input boolean element")
        e.assignmentOptional().defaultValue(True)
        e.reconfigurable()
        e.commit()
	
	e = VECTOR_INT32_ELEMENT(expected)
        e.key("myVectorInt32")
	e.alias(25)
        e.displayedName("myVectorInt32Element")
        e.description("Input vector Int32 element")
        e.assignmentOptional().noDefaultValue()
        e.reconfigurable()
        e.commit()

        e = VECTOR_INT32_ELEMENT(expected)
        e.key("myVectorInt32NEW")
        e.displayedName("myVectorInt32ElementNew")
        e.description("Input vector Int32 element New")
        e.assignmentOptional().defaultValue([1,2,3,4,5])
        e.reconfigurable()
        e.commit()
	
	e = VECTOR_INT32_ELEMENT(expected)
        e.key("myVectorInt32NEXT")
        e.displayedName("myVectorInt32ElementNEXT")
        e.description("Input vector Int32 element NEXT")
        e.assignmentOptional().defaultValueFromString("10,20,30,50")
        e.reconfigurable()
        e.commit()

	e = VECTOR_STRING_ELEMENT(expected)
        e.key("myVectorString")
        e.displayedName("myVectorString")
        e.description("Input vector string element")
        e.assignmentOptional().defaultValue(['Hallo World','next word'])
        e.reconfigurable()
        e.commit()
	
	e = VECTOR_STRING_ELEMENT(expected)
        e.key("myVectorStringNew")
        e.displayedName("myVectorStringNew")
        e.description("Input vector string element New")
        e.assignmentOptional().defaultValueFromString("Hallo World, next word")
        e.reconfigurable()
        e.commit()
	
	e = VECTOR_INT32_ELEMENT(expected)
        e.key("myVectorIntReadOnly")
        e.displayedName("myVectorIntReadOnly")
        e.description("Input vector Int element ReadOnly")
        e.readOnly().initialValueFromString("1,2,3")
        e.commit()
	
	e = VECTOR_BOOL_ELEMENT(expected)
        e.key("myVectorBool")
        e.displayedName("myVectorBool")
        e.description("Input vector Bool")
	e.minSize(2)
	e.maxSize(5)
        e.assignmentOptional().defaultValue([True,False,True])
	e.reconfigurable()
        e.commit()

'''
Test cases in Python:
>python
>>>from libkarathon import *
>>>from schemaTest import *

>>>schA=Schema()
>>>schB=Schema("mySchema")

>>>master=Schema("abc", AssemblyRules(AccessType(READ | WRITE | INIT)))

>>>TestClass2.expectedParameters(master)
>>> print master
Schema for: abc
myInt displayedName="myIntegerElement" description="Input first integer element" assignment="0" defaultValue="5" accessMode="4" nodeType="0" valueType="INT32" => 0 INT32
myIntNew displayedName="myIntegerElementNew" description="Input first integer element new" accessMode="2" assignment="0" defaultValue="0" nodeType="0" valueType="INT32" => 0 INT32

>>> master.help()

----- HELP -----
Schema: abc

  .myInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input first integer element
     Access mode: reconfigurable

  .myIntNew(INT32)
     Assignment : OPTIONAL
     Default value : 0
     Description : Input first integer element new
     Access mode: read only
>>> 

IF you use @schemamethod like in TestClass1, THEN:

>>> master1 = TestClass1.expectedParameters()
>>> master1.help()

----- HELP -----
Schema: TestClass1

  .myInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input first integer element
     Access mode: reconfigurable

  .myIntNew(INT32)
     Assignment : OPTIONAL
     Default value : 0
     Description : Input first integer element new
     Access mode: read only

>>> print master1
Schema for: TestClass1
myInt displayedName="myIntegerElement" description="Input first integer element" assignment="0" defaultValue="5" accessMode="4" nodeType="0" valueType="INT32" => 0 INT32
myIntNew displayedName="myIntegerElementNew" description="Input first integer element new" accessMode="2" assignment="0" defaultValue="0" nodeType="0" valueType="INT32" => 0 INT32

>>>
'''
'''
==========  TESTING NODE_ELEMENT ==============
>>> from schema_test import *
>>> master1.help()
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
NameError: name 'master1' is not defined
>>> from libkarathon import *
>>> master1 = TestClass1.expectedParameters()
>>> print master
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
NameError: name 'master' is not defined
>>> print master1
Schema for: TestClass1
myInt displayedName="myIntegerElement" description="Input first integer element" assignment="0" defaultValue="5" accessMode="4" nodeType="0" valueType="INT32" => 0 INT32
myIntNew displayedName="myIntegerElementNew" description="Input first integer element new" accessMode="2" assignment="0" defaultValue="0" nodeType="0" valueType="INT32" => 0 INT32
MyNodeElement tags="myNode" displayedName="MyNodeElem" description="Description of my node elem" accessMode="7" nodeType="1" +
  a tags="myNode" displayedName="MyNodeElem_A" description="Description of my node elem A" assignment="1" nodeType="0" valueType="DOUBLE" accessMode="1" => 0 INT32

>>> master1.help()

----- HELP -----
Schema: TestClass1

  .myInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input first integer element
     Access mode: reconfigurable

  .myIntNew(INT32)
     Assignment : OPTIONAL
     Default value : 0
     Description : Input first integer element new
     Access mode: read only

  .MyNodeElement(NODE)
     Description : Description of my node elem
>>> master1.help('MyNodeElement')

----- HELP -----
Schema: TestClass1 , key: MyNodeElement
NODE element

  .a(DOUBLE)
     Assignment : MANDATORY
     Description : Description of my node elem A
     Access mode: initialization
>>>
'''
'''
=========== LIST_ELEMENT  =========================
>>> from libkarathon import *
>>> from schema_test import *
>>> master1 = TestClass1.expectedParameters()
>>> print master1
Schema for: TestClass1
myInt displayedName="myIntegerElement" description="Input first integer element" assignment="0" defaultValue="5" accessMode="4" nodeType="0" valueType="INT32" => 0 INT32
myIntNew displayedName="myIntegerElementNew" description="Input first integer element new" accessMode="2" assignment="0" defaultValue="0" nodeType="0" valueType="INT32" => 0 INT32
MyNodeElement tags="myNode" displayedName="MyNodeElem" description="Description of my node elem" accessMode="7" nodeType="1" +
  a tags="myNode" displayedName="MyNodeElem_A" description="Description of my node elem A" assignment="1" nodeType="0" valueType="DOUBLE" accessMode="1" => 0 INT32
shapes displayedName="shapesAsList" description="Description of List-element shapes" min="2" max="5" assignment="1" accessMode="7" nodeType="3" +
  circle tags="shape" displayedName="Circle" description="Description of circle" accessMode="7" nodeType="1" +
    radius tags="shape" displayedName="radius" description="Radius of circle" minInc="5" maxExc="10" assignment="0" defaultValue="5" nodeType="0" valueType="INT32" accessMode="1" => 0 INT32
  rectangle tags="shape" displayedName="rectangle" description="Description of rectangle" accessMode="7" nodeType="1" +
    square tags="shape" displayedName="square" description="Description of square of rectangle" assignment="0" nodeType="0" valueType="DOUBLE" accessMode="1" => 0 INT32

>>> master1.help()

----- HELP -----
Schema: TestClass1

  .myInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input first integer element
     Access mode: reconfigurable

  .myIntNew(INT32)
     Assignment : OPTIONAL
     Default value : 0
     Description : Input first integer element new
     Access mode: read only

  .MyNodeElement(NODE)
     Description : Description of my node elem

  .shapes(LIST_OF_NODES)
     Assignment : MANDATORY
     Description : Description of List-element shapes
>>> master1.help('shapes')

----- HELP -----
Schema: TestClass1 , key: shapes
LIST element

  .circle(NODE)
     Description : Description of circle

  .rectangle(NODE)
     Description : Description of rectangle
>>> master1.help('shapes.circle')

----- HELP -----
Schema: TestClass1 , key: shapes.circle
NODE element

  .radius(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Radius of circle
     Access mode: initialization
>>> 
'''
'''
================ CHOICE_ELEMENT ===========================
>>> from schema_test import *
>>> from libkarathon import *
>>> master1 = TestClass1.expectedParameters()
>>> print master1
Schema for: TestClass1                                                                               
myInt displayedName="myIntegerElement" description="Input first integer element" assignment="0" defaultValue="5" accessMode="4" nodeType="0" valueType="INT32" => 0 INT32
myIntNew displayedName="myIntegerElementNew" description="Input first integer element new" accessMode="2" assignment="0" defaultValue="0" nodeType="0" valueType="INT32" => 0 INT32                       
MyNodeElement tags="myNode" displayedName="MyNodeElem" description="Description of my node elem" accessMode="7" nodeType="1" +
  a tags="myNode" displayedName="MyNodeElem_A" description="Description of my node elem A" assignment="1" nodeType="0" valueType="DOUBLE" accessMode="1" => 0 INT32                                       
shapes displayedName="shapesAsList" description="Description of List-element shapes" assignment="1" accessMode="7" nodeType="2" +                                                                         
  circle tags="shape" displayedName="Circle" description="Description of circle" accessMode="7" nodeType="1" +                                                                                            
    radius tags="shape" displayedName="radius" description="Radius of circle" minInc="5" maxExc="10" assignment="0" defaultValue="5" nodeType="0" valueType="INT32" accessMode="1" => 0 INT32             
  rectangle tags="shape" displayedName="rectangle" description="Description of rectangle" accessMode="7" nodeType="1" +
    square tags="shape" displayedName="square" description="Description of square of rectangle" assignment="0" nodeType="0" valueType="DOUBLE" accessMode="1" => 0 INT32

>>> master1.help()

----- HELP -----
Schema: TestClass1

  .myInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input first integer element
     Access mode: reconfigurable

  .myIntNew(INT32)
     Assignment : OPTIONAL
     Default value : 0
     Description : Input first integer element new
     Access mode: read only

  .MyNodeElement(NODE)
     Description : Description of my node elem

  .shapes(CHOICE_OF_NODES)
     Assignment : MANDATORY
     Description : Description of List-element shapes
>>> master1.help('shapes')

----- HELP -----
Schema: TestClass1 , key: shapes
CHOICE element

  .circle(NODE)
     Description : Description of circle

  .rectangle(NODE)
     Description : Description of rectangle
>>> master1.help('shapes.rectangle')

----- HELP -----
Schema: TestClass1 , key: shapes.rectangle
NODE element

  .square(DOUBLE)
     Assignment : OPTIONAL
     Description : Description of square of rectangle
     Access mode: initialization
>>>
'''
'''
============= testing OutputSchema =============
master1 = TestClass1.expectedParameters()
schemaToXsd(master1, "/tmp/TestClass1.xsd")
'''



