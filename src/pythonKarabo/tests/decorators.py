# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
from karabo_decorators import *

@KARABO_CLASSINFO("Example", "1.0")
@KARABO_CONFIGURATION_BASE_CLASS
class ExampleClass(object): pass

class  DecoratorsTestCase(unittest.TestCase):
    #def setUp(self):
    #    self.foo = Decorators()
    #

    #def tearDown(self):
    #    self.foo.dispose()
    #    self.foo = None

    def test_decorators(self):
        #assert x != y;
        #self.assertEqual(x, y, "Msg");
        self.fail("TODO: Write test")

if __name__ == '__main__':
    unittest.main()

