import unittest
import numpy as np
from libkarathon import *


class  AttributesTestCase(unittest.TestCase):

    def test_attributes(self):
        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
        except Exception,e:
            self.fail("test_attributes exception group 1: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
            
            h.setAttribute("a.b.a.b", "attr2", 42)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 42, 'Should return 42')
            
            h.setAttribute("a.b.a.b", "attr2", 43)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43, 'Should return 42')
            
            h.setAttribute("a.b.a.b", "attr1", True)
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True, 'Should return "someValue"')
            self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43, 'Should return 42')
            
            attrs = h.getAttributes("a.b.a.b")
            self.assertEqual(attrs.size(), 2)
            self.assertEqual(attrs.get("attr1"), True)
            self.assertEqual(attrs.get("attr2"), 43)
            
            node = attrs.getNode("attr1")
            self.assertEqual(node.getType(), "BOOL")

            node = attrs.getNode("attr2")
            self.assertEqual(node.getType(), "INT32")

        except Exception,e:
            self.fail("test_attributes exception group 2: " + str(e))

        try:
            h = Hash("a.b.a.b", 1)
            h.setAttribute("a.b.a.b","attr1", [1,2,3])
            self.assertEqual(h.getAttribute("a.b.a.b","attr1")[1], 2)
        except Exception,e:
            self.fail("test_attributes exception group 3: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
        except Exception,e:
            self.fail("test_attributes exception group 4: " + str(e))

        try:
            h = Hash("a.b.a.b", 42)
            h.setAttribute("a.b.a.b","attr1", "someValue")
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue", 'Should return "someValue"')
        except Exception,e:
            self.fail("test_attributes exception group 5: " + str(e))


if __name__ == '__main__':
    unittest.main()

