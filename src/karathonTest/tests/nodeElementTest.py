'''
Created on April 11, 2013

@author: irinak
'''

from libkarathon import *

class SomeCustomerClass(object):  	
    @classmethod
    def expectedParameters(self, expected):
        e = INT32_ELEMENT(expected)
        e.key("mySubInt")
        e.displayedName("mySubIntegerElement")
        e.description("Input sub-integer element")
        e.assignmentOptional().defaultValue(5)
        e.reconfigurable()
        e.commit()
	
	e = STRING_ELEMENT(expected)
        e.key("mySubString")
        e.displayedName("mySubStringElement")
        e.description("Input sub-string element")
        e.assignmentOptional().noDefaultValue()
        e.reconfigurable()
        e.commit()
    
    @classmethod
    def getSchema(self):
        s=Schema("SomeCustomerClass", AssemblyRules(AccessType(READ | WRITE | INIT)))
	self.expectedParameters(s)
        return s 

'''
Class for testing NODE_ELEMENT schema element with function 'appendParametersOf(customerClass)'
'''
class TestClass(object):
    @classmethod
    def expectedParameters(self, expected):
        e = INT32_ELEMENT(expected)
        e.key("myInt")
        e.displayedName("myIntegerElement")
        e.description("Input first integer element")
        e.assignmentOptional().defaultValue(5)
        e.reconfigurable()
        e.commit()
	
	e = NODE_ELEMENT(expected)
        e.key("SomeCustomerElement")
        e.displayedName("someCustomerElement")
        e.description("someCustomerElement description")
        e.appendParametersOf(SomeCustomerClass)
	e.commit()
        
'''
Test case in Python:
>python
>>>from nodeElementTest import *
>>>master=Schema("mySchema")
>>>TestClass.expectedParameters(master)
>>>master.help()

----- HELP -----
Schema: mySchema

  .myInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input first integer element
     Access mode: reconfigurable

  .SomeCustomerElement(NODE)
     Description : someCustomerElement description

>>>master.help("SomeCustomerElement")

----- HELP -----
Schema: mySchema , key: SomeCustomerElement
NODE element

  .mySubInt(INT32)
     Assignment : OPTIONAL
     Default value : 5
     Description : Input sub-integer element
     Access mode: reconfigurable

  .mySubString(STRING)
     Assignment : OPTIONAL
     Description : Input sub-string element
     Access mode: reconfigurable
>>> 

'''        