import unittest
import numpy as np
from hash import Hash

class  AttributesTestCase(unittest.TestCase):

    def test_attributes_1(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")

    def test_attributes_2(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")
            
        h.setAttribute("a.b.a.b", "attr2", 42)
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 42)
            
        h.setAttribute("a.b.a.b", "attr2", 43)
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43)
            
        h.setAttribute("a.b.a.b", "attr1", True)
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), True)
        self.assertEqual(h.getAttribute("a.b.a.b","attr2"), 43)
            
        attrs = h.getAttributes("a.b.a.b")
        self.assertEqual(attrs.size(), 2)
        self.assertEqual(attrs.get("attr1"), True)
        self.assertEqual(attrs.get("attr2"), 43)
            
        node = attrs.getNode("attr1")
        self.assertEqual(node.getType(), "BOOL")

        node = attrs.getNode("attr2")
        self.assertEqual(node.getType(), "INT32")

    def test_attributes_3(self):
        h = Hash("a.b.a.b", 1)
        h.setAttribute("a.b.a.b","attr1", [1,2,3])
        self.assertEqual(h.getAttribute("a.b.a.b","attr1")[1], 2)

    def test_attributes_4(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")

    def test_attributes_5(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", "someValue")
        self.assertEqual(h.getAttribute("a.b.a.b","attr1"), "someValue")

    def test_attributes_6(self):
        h = Hash("a.b.a.b", 42)
        h.setAttribute("a.b.a.b","attr1", [1,2,3,4,5,6,7])
            
        setStdVectorDefaultConversion(Types.PYTHON)
        if isStdVectorDefaultConversion(Types.PYTHON):
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), [1,2,3,4,5,6,7])
        if isStdVectorDefaultConversion(Types.NUMPY):
            self.assertEqual(h.getAttribute("a.b.a.b","attr1").all(), np.array([1,2,3,4,5,6,7], dtype=np.int32).all())

        setStdVectorDefaultConversion(Types.NUMPY)
        if isStdVectorDefaultConversion(Types.PYTHON):
            self.assertEqual(h.getAttribute("a.b.a.b","attr1"), [1,2,3,4,5,6,7])
        if isStdVectorDefaultConversion(Types.NUMPY):
            self.assertEqual(h.getAttribute("a.b.a.b","attr1").all(), np.array([1,2,3,4,5,6,7], dtype=np.int32).all())

        setStdVectorDefaultConversion(Types.PYTHON)
            
    def test_attributes_7(self):
        h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
        h.setAttribute('a.b.c','attr1',[1.234,2.987,5.555])
        if isStdVectorDefaultConversion(Types.PYTHON):
            self.assertEqual(h.getAttribute('a.b.c','attr1'), [1.234,2.987,5.555])
            self.assertEqual(h.getAttributeAs('a.b.c','attr1',Types.NDARRAY_DOUBLE).all(), np.array([1.234,2.987,5.555], dtype=np.double).all())
        if isStdVectorDefaultConversion(Types.NUMPY):
            self.assertEqual(h.getAttribute('a.b.c','attr1').all(), np.array([1.234,2.987,5.555], dtype=np.double).all())
            self.assertEqual(h.getAttributeAs('a.b.c','attr1',Types.VECTOR_DOUBLE), [1.234,2.987,5.555])

if __name__ == '__main__':
    unittest.main()

