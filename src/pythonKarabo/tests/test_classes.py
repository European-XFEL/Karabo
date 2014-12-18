import sys
sys.karabo_api = 2

import unittest

from karabo.enums import Assignment, AccessMode, Unit, MetricPrefix
from karabo.schema import Configurable, Node
from karabo import hashtypes as ht

class Classes_Test(unittest.TestCase):
    called = None

    def test_simple(self):
        class A(Configurable):
            i = ht.Int32(minExc=7, maxExc=10)
            f = ht.Float(displayedName="hallo", defaultValue=7.3)

        a = A()
        a.i = 3
        self.assertEqual(a.i, 3)
        def setter():
            a.i = "b"
        self.assertRaises(ValueError, setter)
        self.assertAlmostEqual(a.f, 7.3, places=5)

    def test_shape(self):
        class Shape(Configurable):
            shadowEnabled = ht.Bool(
                description="Shadow enabled", displayedName="Shadow",
                assignment=Assignment.OPTIONAL, defaultValue=False,
                accessMode=AccessMode.INITONLY)
        s = Shape(dict(shadowEnabled=True))
        self.assertTrue(s.shadowEnabled)

        class Circle(Shape):
            radius = ht.Double(
                alias=1, description="The radius of the circle",
                displayedName="Radius", minExc=0, maxExc=100,
                unitSymbol=Unit.METER, metricPrefixSymbol=MetricPrefix.MILLI,
                assignment=Assignment.OPTIONAL, defaultValue=10,
                accessMode=AccessMode.INITONLY)
        c = Circle()
        self.assertFalse(c.shadowEnabled)
        self.assertEqual(c.radius, 10)

    def test_composition(self):
        class Inner(Configurable):
            f = ht.Float(defaultValue=7)
        class Outer(Configurable):
            inner = Node(Inner, displayedName="InneR")

        o = Outer()
        self.assertEqual(o.inner.f, 7)
        o.inner.f = 9
        self.assertEqual(o.inner.f, 9)

    def callback(self, *args):
        self.assertIsNone(self.called)
        self.called = args

    def assertCalled(self, *args):
        self.assertEqual(self.called, args)
        self.called = None

    def test_setChild(self):
        oself = self
        class Inner(Configurable):
            f = ht.Float()
        class Outer(Configurable):
            inner = Node(Inner)
            def setChildValue(self, key, value):
                oself.callback(key, value)
        o = Outer()
        o.inner.f = 3
        self.assertCalled("inner.f", 3)


if __name__ == "__main__":
    unittest.main()
