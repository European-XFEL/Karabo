# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="esenov"
__date__ ="$Aug 2, 2012 12:41:16 PM$"

import unittest
from exfel_decorators import schemamethod
from libpyexfel import *

class ExampleClass(object):

    @schemamethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected)
        e.key('firstWord').displayedName("First Word").description("Input for first word")
        e.assignmentOptional().defaultValue("").unitName("dimensionless").unitSymbol("")
        e.reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("secondWord").displayedName("Second Word").description("Input for first word")
        e.assignmentOptional().defaultValue("").unitName("dimensionless").unitSymbol("")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("multiply").displayedName("Multiply").description("multiplies word")
        e.assignmentOptional().defaultValue(1).unitName("#").unitSymbol("#")
        e.allowedStates("ErrorState").reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("composedWord").displayedName("Composed word").description("The composed word")
        e.assignmentOptional().noDefaultValue().unitName("dimensionless").unitSymbol("")
        e.readOnly().commit()


class CheckExpectedParameters(unittest.TestCase):

    def runTest(self):
        print "\n================================= CheckExpectedParameters =================================\n"
        schema = ExampleClass.expectedParameters()
        print schema
        schema = ExampleClass.expectedParameters(AccessType(INIT|READ|WRITE))
        print schema

if __name__ == "__main__":
    unittest.main()
