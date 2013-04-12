# To change this template, choose Tools | Templates
# and open the template in the editor.

from karabo_decorators import *
from libkarathon import *
import unittest

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Example", "1.0")
class ExampleClass(object):

    @staticmethod
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


class  Decorators_TestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Decorators_()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_decorators_(self):
        print "Hello decorators"
        schema = Configurator('Example').getSchema('Example')
        print schema
        #assert x != y;
        #self.assertEqual(schema.hasKey("firstWord"),    True, "expectedParameters failed -- no 'firstWord' key found")
        #self.assertEqual(schema.hasKey("secondWord"),   True, "expectedParameters failed -- no 'secondWord' key found")
        #self.assertEqual(schema.hasKey("multiply"),     True, "expectedParameters failed -- no 'multiply' key found")
        #self.assertEqual(schema.hasKey("composedWord"), True, "expectedParameters failed -- no 'composedWord' key found")
        #self.assertEqual(schema.hasKey("thirdWord"),    False,"expectedParameters failed -- 'thirdWord' key was found")
        #self.fail("TODO: Write test")

if __name__ == '__main__':
    unittest.main()

