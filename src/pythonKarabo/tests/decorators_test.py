# To change this template, choose Tools | Templates
# and open the template in the editor.

from karabo.decorators import *
from karabo.karathon import *
import unittest

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("ExampleBase", "1.0")
class ExampleBaseClass(object):

    def __init__(self, configuration):
        self.configuration = configuration

    @staticmethod
    def expectedParameters(expected):
    
        e = STRING_ELEMENT(expected)
        e.key('firstWord').displayedName("First Word").description("Input for first word")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("secondWord").displayedName("Second Word").description("Input for first word")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("multiply").displayedName("Multiply").description("multiplies word")
        e.assignmentOptional().defaultValue(1)
        e.allowedStates("ErrorState").reconfigurable().commit()

        e = STRING_ELEMENT(expected)
        e.key("composedWord").displayedName("Composed word").description("The composed word")
        e.assignmentOptional().noDefaultValue()
        e.readOnly().commit()


@KARABO_CLASSINFO("ExampleDerived", "1.0")
class ExampleDerivedClass(ExampleBaseClass):
    
    def __init__(self, configuration):
        super(ExampleDerivedClass, self).__init__(configuration)
        self.configuration = configuration
    
    @staticmethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected)
        e.key('hostname').displayedName("Hostname").description("Input for host name")
        e.assignmentOptional().defaultValue("")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("port").displayedName("Port").description("Input for port")
        e.assignmentOptional().defaultValue(2999)
        e.reconfigurable().commit()


@KARABO_CLASSINFO("Example", "1.0")
class ExampleClass(ExampleDerivedClass):
    
    def __init__(self, configuration):
        super(ExampleClass, self).__init__(configuration)
        self.configuration = configuration
    
    @staticmethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected)
        e.key('street').displayedName("Street").description("Street")
        e.assignmentOptional().defaultValue("Albert-Einstein-Ring")
        e.reconfigurable().commit()

        e = INT32_ELEMENT(expected)
        e.key("bld").displayedName("Bld").description("Building number")
        e.assignmentOptional().defaultValue(19)
        e.reconfigurable().commit()


class  Decorators_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Decorators_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_decorators_(self):
        try:
            example = Configurator('ExampleBase').create('Example', Hash())
            example2 = ExampleBaseClass.create('Example', Hash())
            schema = Configurator("ExampleBase").getSchema('Example')     # call Configurator by classid
            schema = Configurator(ExampleBaseClass).getSchema('Example')  # call Configurator by class
            self.assertEqual(schema.getValueType("firstWord"),  Types.STRING, "expectedParameters failed -- no 'firstWord' key found")
            self.assertEqual(schema.getValueType("secondWord"), Types.STRING, "expectedParameters failed -- no 'secondWord' key found")
            self.assertEqual(schema.getValueType("multiply"),   Types.INT32, "expectedParameters failed -- no 'multiply' key found")
            self.assertEqual(schema.getValueType("composedWord"), Types.STRING, "expectedParameters failed -- no 'composedWord' key found")
            self.assertEqual(schema.getValueType("hostname"),   Types.STRING)
            self.assertEqual(schema.getValueType("port"),       Types.INT32)
            self.assertEqual(schema.getValueType("street"),     Types.STRING)

        except Exception as e:
            self.fail("test_decorators group 1: " + str(e))

if __name__ == '__main__':
    unittest.main()

