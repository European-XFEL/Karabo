import asyncio
from unittest import TestCase, main
import string
from zlib import adler32

import numpy

from karabo.middlelayer import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, Bool, Configurable,
    ComplexDouble, ComplexFloat, DaqDataType, DaqPolicy, decodeBinary,
    DeviceNode, Double, encodeBinary, Float, Hash, Image, ImageData, Int8,
    Int16, Int32, Int64, isSet, KaraboError, MetricPrefix, Node, Overwrite,
    RegexString, Slot, State, String, UInt8, UInt16, UInt32, UInt64,
    Unit, unit, VectorBool, VectorChar, VectorComplexDouble,
    VectorComplexFloat, VectorDouble, VectorHash, VectorFloat, VectorInt8,
    VectorInt16, VectorInt32, VectorInt64, VectorString, VectorUInt8,
    VectorUInt16, VectorUInt32, VectorUInt64)
from ..injectable import InjectMixin


class StoreChanges(Configurable):
    """This test class keeps all changes for later inspection"""
    values_set = None
    children_set = None

    def setValue(self, desc, value):
        super().setValue(desc, value)
        if self.values_set is None:
            self.values_set = []
        self.values_set.append((desc, value))

    def setChildValue(self, key, value, desc):
        super().setChildValue(key, value, desc)
        if self.children_set is None:
            self.children_set = []
        self.children_set.append((key, value, desc))


def rehash(**kwargs):
    """assure hashes look exactly like they came over the network"""
    h = Hash(kwargs)
    return decodeBinary(encodeBinary(h))


def run_coro(coro):
    loop = asyncio.get_event_loop()
    return loop.run_until_complete(coro)


class Tests(TestCase):
    @classmethod
    def setUpClass(cls):
        asyncio.set_event_loop(asyncio.new_event_loop())

    def test_nodefault(self):
        class B(Configurable):
            bvalue = Int32(unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("bvalue", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.bvalue, 5 * unit.meter)

    def test_default(self):
        class B(Configurable):
            bvalue = Int32(unitSymbol=Unit.METER, defaultValue=33)

        class A(StoreChanges):
            value = Int32(unitSymbol=Unit.METER, defaultValue=22)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.bvalue, 33 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 22 * unit.meter),
                                        (A.node, a.node)])

        a.values_set = []
        self.assertEqual(a.children_set, [("value", 22 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("bvalue", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.bvalue, 5 * unit.meter)

    def test_mandatory(self):
        class B(Configurable):
            bvalue = Int32(assignment=Assignment.MANDATORY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        with self.assertRaises(KaraboError):
            a = A(rehash(node=Hash("bvalue", 3)))
        with self.assertRaises(KaraboError):
            a = A(rehash(value=7))

        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        run_coro(a.slotReconfigure(rehash(value=10, node=Hash("bvalue", 5))))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.node.bvalue, 5 * unit.meter)

    def test_readonly(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.READONLY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        # we ignore read only parameters in configuration
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_readonly_default(self):
        class B(Configurable):
            bvalue = Int32(defaultValue=9,
                           accessMode=AccessMode.READONLY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(defaultValue=5,
                          accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 5 * unit.meter)
        self.assertEqual(a.node.bvalue, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 5 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 5 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        # we ignore read only parameters in configuration
        self.assertEqual(a.values_set, [(A.value, 5 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 5 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_init_nodefault(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.INITONLY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertEqual(a.values_set, [(A.value, None), (A.node, a.node)])
        self.assertEqual(a.children_set, [("node", a.node, A.node)])
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_init_default(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.INITONLY,
                           unitSymbol=Unit.METER, defaultValue=33)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.INITONLY,
                          unitSymbol=Unit.METER, defaultValue=22)
            node = Node(B)

        a = A()
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.bvalue, 33 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 22 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 22 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.values_set, [(A.value, 7 * unit.meter),
                                        (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 7 * unit.meter, A.value),
                                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))

    def test_init_mandatory(self):
        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.INITONLY,
                           assignment=Assignment.MANDATORY,
                           unitSymbol=Unit.METER)

        class A(StoreChanges):
            value = Int32(accessMode=AccessMode.INITONLY,
                          assignment=Assignment.MANDATORY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        with self.assertRaises(KaraboError):
            a = A(rehash(node=Hash("bvalue", 3)))
        with self.assertRaises(KaraboError):
            a = A(rehash(value=7))

        a = A(rehash(value=7, node=Hash("bvalue", 3)))
        self.assertEqual(a.value, 7 * unit.meter)
        self.assertEqual(a.node.bvalue, 3 * unit.meter)
        self.assertEqual(a.values_set,
                         [(A.value, 7 * unit.meter), (A.node, a.node)])
        a.values_set = []
        self.assertEqual(a.children_set,
                         [("value", 7 * unit.meter, A.value),
                          ("node", a.node, A.node)])
        a.children_set = []
        a.value = 9 * unit.meter
        self.assertEqual(a.value, 9 * unit.meter)
        self.assertEqual(a.values_set, [(A.value, 9 * unit.meter)])
        a.values_set = []
        self.assertEqual(a.children_set, [("value", 9 * unit.meter, A.value)])
        a.children_set = []
        a.node.bvalue = 4 * unit.meter
        self.assertEqual(a.node.bvalue, 4 * unit.meter)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set,
                         [("node.bvalue", 4 * unit.meter, B.bvalue)])
        a.children_set = []
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(value=10)))
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(rehash(node=Hash("bvalue", 5))))

    def test_setter(self):
        setter_value = init_value = 0

        class Setter(Int32):
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                super().initialize(instance, value + 2 * unit.meter)

        class A(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        a = A(rehash(value=44))
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=3)))
        self.assertEqual(setter_value, (44 + 3 + 2) * unit.meter)
        self.assertEqual(a.value, (3 + 1) * unit.meter)

    def test_setter_nested(self):
        setter_value = init_value = 0

        class Setter(Int32):
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                super().initialize(instance, value + 2 * unit.meter)

        class B(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        class A(Configurable):
            node = Node(B)

        a = A(rehash(node=Hash("value", 44)))
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.node.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(node=Hash("value", 3))))
        self.assertEqual(setter_value, (44 + 3 + 2) * unit.meter)
        self.assertEqual(a.node.value, (3 + 1) * unit.meter)

    def test_setter_coro(self):
        setter_value = init_value = 0

        class Setter(Int32):
            @asyncio.coroutine
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            @asyncio.coroutine
            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                yield from super().initialize(instance, value + 2 * unit.meter)

        class A(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        a = A(rehash(value=44))
        self.assertEqual(init_value, 0)
        self.assertEqual(setter_value, 0)
        run_coro(a._run())
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(value=3)))
        self.assertEqual(setter_value, (44 + 2 + 3) * unit.meter)
        self.assertEqual(a.value, 4 * unit.meter)

    def test_setter_coro_nested(self):
        setter_value = init_value = 0

        class Setter(Int32):
            @asyncio.coroutine
            def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            @asyncio.coroutine
            def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                yield from super().initialize(instance, value + 2 * unit.meter)

        class B(Configurable):
            value = Setter(unitSymbol=Unit.METER)

        class A(Configurable):
            node = Node(B)

        a = A(rehash(node=Hash("value", 44)))
        self.assertEqual(init_value, 0)
        self.assertEqual(setter_value, 0)
        run_coro(a._run())
        self.assertEqual(init_value, 44 * unit.meter)
        self.assertEqual(setter_value, (44 + 2) * unit.meter)
        self.assertEqual(a.node.value, (44 + 2 + 1) * unit.meter)
        run_coro(a.slotReconfigure(rehash(node=Hash("value", 3))))
        self.assertEqual(setter_value, (44 + 2 + 3) * unit.meter)
        self.assertEqual(a.node.value, 4 * unit.meter)

    def test_cross_error(self):
        class B(Configurable):
            allowed = Int32(defaultValue=1)
            forbidden = Int32(accessMode=AccessMode.READONLY,
                              defaultValue=2)

        class A(Configurable):
            allowed = Int32(defaultValue=3)
            forbidden = Int32(accessMode=AccessMode.READONLY,
                              defaultValue=4)
            node = Node(B)

        a = A()
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(
                rehash(allowed=5, forbidden=6)))
        self.assertEqual(a.allowed, 3)
        self.assertEqual(a.node.allowed, 1)
        with self.assertRaises(KaraboError):
            run_coro(a.slotReconfigure(
                rehash(node=Hash("allowed", 7, "forbidden", 8))))
        self.assertEqual(a.allowed, 3)
        self.assertEqual(a.node.allowed, 1)

    def test_nested(self):
        class C(Configurable):
            value = Int32(defaultValue=3)

        class B(Configurable):
            nested = Node(C)

        class A(StoreChanges):
            nested = Node(B)

        a = A()
        self.assertEqual(a.values_set, [(A.nested, a.nested)])
        a.values_set = []
        self.assertEqual(a.children_set, [("nested", a.nested, A.nested)])
        a.children_set = []
        self.assertEqual(a.nested.nested.value, 3)
        a.nested.nested.value = 5
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set, [("nested.nested.value", 5, C.value)])
        a.children_set = []
        self.assertEqual(a.nested.nested.value, 5)
        run_coro(a.slotReconfigure(rehash(
            nested=Hash("nested", Hash("value", 7)))))
        self.assertEqual(a.nested.nested.value, 7)
        self.assertEqual(a.values_set, [])
        self.assertEqual(a.children_set, [("nested.nested.value", 7, C.value)])

    def test_vector_size(self):
        class A(Configurable):
            stringVector = VectorString(
                defaultValue=['bla', 'blub'],
                minSize=2)

            floatVector = VectorFloat(
                defaultValue=[1, 2, 3],
                minSize=2,
                maxSize=4)

            boolVector = VectorBool(
                defaultValue=[True, False],
                minSize=2,
                maxSize=4)

            charVector = VectorChar(
                defaultValue=[1, 2],
                minSize=2,
                maxSize=4)

            doubleVector = VectorDouble(
                defaultValue=[1.2, 1.3],
                minSize=2,
                maxSize=4)

            intVector = VectorInt32(
                defaultValue=[1, 2, 3],
                minSize=2,
                maxSize=4)

            complexVector = VectorComplexFloat(
                defaultValue=[2 + 3j, 3 + 4j, 4],
                maxSize=4)

        a = A()
        self.assertEqual(a.stringVector.descriptor.minSize, 2)
        self.assertEqual(a.floatVector.descriptor.minSize, 2)
        self.assertEqual(a.floatVector.descriptor.maxSize, 4)
        self.assertEqual(a.boolVector.descriptor.minSize, 2)
        self.assertEqual(a.boolVector.descriptor.maxSize, 4)
        self.assertEqual(a.charVector.descriptor.minSize, 2)
        self.assertEqual(a.charVector.descriptor.maxSize, 4)
        self.assertEqual(a.doubleVector.descriptor.minSize, 2)
        self.assertEqual(a.doubleVector.descriptor.maxSize, 4)
        self.assertEqual(a.intVector.descriptor.minSize, 2)
        self.assertEqual(a.intVector.descriptor.maxSize, 4)
        self.assertEqual(a.complexVector.descriptor.maxSize, 4)

        with self.assertRaises(ValueError):
            a.stringVector = ['miau']
        a.stringVector = ['miau', 'wuff']
        self.assertEqual(a.stringVector, ['miau', 'wuff'])

        with self.assertRaises(ValueError):
            a.floatVector = [1]
        with self.assertRaises(ValueError):
            a.floatVector = [1] * 5
        a.floatVector = [1, 27, 35]
        # pint differences
        self.assertEqual(a.floatVector.value[0], 1)
        self.assertEqual(a.floatVector.value[2], 35)
        with self.assertRaises(ValueError):
            a.floatVector = ['nofloat', 'float?']

        with self.assertRaises(ValueError):
            a.boolVector = [False]
        a.boolVector = [False, True, True]
        self.assertEqual(a.boolVector.value[0], False)
        self.assertEqual(a.boolVector.value[1], True)
        self.assertEqual(a.boolVector.value[2], True)

        with self.assertRaises(ValueError):
            a.charVector = [1]
        with self.assertRaises(ValueError):
            a.charVector = [1, 2, 3, 4, 5]
        a.charVector = [1, 2]
        self.assertEqual(a.charVector.value[0], 1)
        self.assertEqual(a.charVector.value[1], 2)

        with self.assertRaises(ValueError):
            a.doubleVector = [1.2]
        with self.assertRaises(ValueError):
            a.doubleVector = [1.7, 2.1, 3.6, 4.12, 5.0]
        a.doubleVector = [1.2, 2.5]
        self.assertEqual(a.doubleVector.value[0], 1.2)
        self.assertEqual(a.doubleVector.value[1], 2.5)

        with self.assertRaises(ValueError):
            a.intVector = [1]
        with self.assertRaises(ValueError):
            a.intVector = [1, 2, 3, 4, 5]
        a.intVector = [17, 5]
        self.assertEqual(a.intVector.value[0], 17)
        self.assertEqual(a.intVector.value[1], 5)

        a.complexVector = [2 + 3j]
        self.assertEqual(a.complexVector.value[0], 2 + 3j)
        with self.assertRaises(ValueError):
            a.complexVector = [2 + 3j, 3 + 4j, 4, 17 + 4j, 1]
        a.complexVector = [5 + 3j, 3 + 4j]
        self.assertEqual(a.complexVector.value[0], 5 + 3j)
        self.assertEqual(a.complexVector.value[1], 3 + 4j)

    def test_table(self):
        class Row(Configurable):
            name = String()

            number = Int32(
                unitSymbol=Unit.METER,
                metricPrefixSymbol=MetricPrefix.MILLI)
            counter = Int32()

            vectorString = VectorString(
                unitSymbol=Unit.METER,
                metricPrefixSymbol=MetricPrefix.MILLI)

        class A(Configurable):
            table = VectorHash(rows=Row, defaultValue=[])

        a = A()
        self.assertEqual(a.table.shape, (0,))
        a.table = [("asf", 3, 5, ["Bart", "Marge"]), ("fw", 2, 5, ["Homer"])]
        self.assertEqual(a.table.shape, (2,))
        self.assertEqual(a.table["name"][1], "fw")

        a.table.append(("lll", 55, 10, ["Lisa"]))
        self.assertEqual(len(a.table), 3)
        self.assertEqual(a.table[2]["name"], "lll")
        self.assertEqual(a.table["number"][2], 55 * unit.millimeter)

        a.table.extend([(str(i), i, i + 1, ["Moe"]) for i in range(10)])
        self.assertEqual(a.table[10]["number"], 7 * unit.millimeter)

        del a.table[10]
        self.assertEqual(a.table[10]["number"], 8 * unit.millimeter)
        self.assertEqual(a.table[10]["counter"], 9)
        del a.table[10:12]
        self.assertEqual(len(a.table), 10)

        a.table[1] = ("k", 111, 72, ["Nelson"])
        self.assertEqual(a.table[1]["name"], "k")
        self.assertEqual(a.table[1]["counter"], 72)
        self.assertEqual(a.table[1]["vectorString"], ["Nelson"])
        self.assertEqual(len(a.table), 10)

        a.table[5:7] = [("A", 2, 1, ["Scratchy"]), ("B", 3, 2, ["Nelson"])]
        self.assertEqual(a.table[5]["name"], "A")
        self.assertEqual(a.table["number"][6], 3 * unit.millimeter)
        self.assertEqual(a.table["number"][7], 4 * unit.millimeter)
        self.assertEqual(a.table["counter"][6], 2)
        self.assertEqual(a.table["counter"][7], 5)
        self.assertEqual(a.table["vectorString"][6], ["Nelson"])
        self.assertEqual(a.table["vectorString"][7], ["Moe"])

        a.table.insert(2, ("C", 11, 5, ["Itchy", "Burns"]))
        self.assertEqual(a.table[2]["name"], "C")
        self.assertEqual(a.table[3]["name"], "lll")
        self.assertEqual(a.table[2]["number"], 11 * unit.millimeter)
        self.assertEqual(a.table[2]["counter"], 5)
        self.assertEqual(a.table[2]["vectorString"], ["Itchy", "Burns"])
        self.assertEqual(len(a.table), 11)

        self.assertEqual(adler32(str(a.table).encode("ascii")), 3585240932)

        a = A(Hash("table", [Hash("name", "bla",
                                  "number", 5, "counter", 2,
                                  "vectorString", ["Itchy", "Burns"])]))
        self.assertEqual(a.table.shape, (1,))
        self.assertEqual(a.table["name"][0], "bla")
        b = a.table.pop()
        self.assertEqual(a.table.shape, (0,))
        self.assertEqual(b["name"], "bla")
        self.assertEqual(b["number"], 5 * unit.millimeter)

        c = A(Hash("table", [Hash("name", "bla",
                                  "number", 5, "counter", 2,
                                  "vectorString", ["Itchy", "Burns"])]))
        self.assertEqual(len(c.table.value), 1)
        c.table.extend([(str(i), i, i + 1, ["Abu"]) for i in range(10)])
        self.assertEqual(len(c.table.value), 11)
        c.table.clear()
        self.assertEqual(len(c.table.value), 0)

        with self.assertRaises(KaraboError):
            class B(Configurable):
                table = VectorHash(defaultValue=[])

    def test_overwrite(self):
        class Mandy(Configurable):
            number = Int32(displayedName="whatever", minExc=7,
                           accessMode=AccessMode.READONLY,
                           allowedStates={State.ON}, tags=set(),
                           unitSymbol=Unit.METER,
                           metricPrefixSymbol=MetricPrefix.MILLI,
                           options=[8, 9, 10])
            cookies = String(allowedStates={State.HOMING},
                             displayedName="else",
                             unitSymbol=Unit.METER,
                             metricPrefixSymbol=MetricPrefix.MILLI,
                             options=["a", "b"])
            state = String(enum=State)

        class Brian(Mandy):
            number = Overwrite(minExc=3, allowedStates={State.OFF},
                               accessMode=AccessMode.INITONLY,
                               unitSymbol=Unit.SECOND,
                               metricPrefixSymbol=MetricPrefix.MEGA,
                               tags={"naughty"},
                               options=[6, 4])
            cookies = Overwrite()
            state = Overwrite(options=[State.ON, State.OFF])

        self.assertEqual(Brian.number.key, "number")
        self.assertEqual(Mandy.number.minExc, 7)
        self.assertEqual(Brian.number.minExc, 3)
        self.assertEqual(Mandy.number.displayedName, "whatever")
        self.assertEqual(Brian.number.displayedName, "whatever")
        self.assertEqual(Mandy.number.allowedStates, {State.ON})
        self.assertEqual(Brian.number.allowedStates, {State.OFF})
        self.assertEqual(Mandy.number.tags, set())
        self.assertEqual(Brian.number.tags, {"naughty"})
        self.assertIs(Mandy.number.accessMode, AccessMode.READONLY)
        self.assertIs(Brian.number.accessMode, AccessMode.INITONLY)
        self.assertIs(Mandy.number.unitSymbol, Unit.METER)
        self.assertIs(Brian.number.unitSymbol, Unit.SECOND)
        self.assertEqual(Mandy.number.units, unit.millimeter)
        self.assertEqual(Brian.number.units, unit.megasecond)
        self.assertEqual(Mandy.number.options, [8, 9, 10])
        self.assertEqual(Brian.number.options, [6, 4])

        self.assertEqual(Mandy.cookies.allowedStates, {State.HOMING})
        self.assertEqual(Brian.cookies.allowedStates, {State.HOMING})
        self.assertEqual(Mandy.cookies.displayedName, "else")
        self.assertEqual(Brian.cookies.displayedName, "else")
        self.assertIs(Mandy.cookies.unitSymbol, Unit.METER)
        self.assertIs(Brian.cookies.unitSymbol, Unit.METER)
        self.assertEqual(Mandy.cookies.units, unit.millimeter)
        self.assertEqual(Brian.cookies.units, unit.millimeter)
        self.assertEqual(Mandy.cookies.options, ["a", "b"])
        self.assertEqual(Brian.cookies.options, ["a", "b"])

        self.assertIsNone(Mandy.state.options)
        self.assertEqual(Brian.state.options, [State.ON, State.OFF])

    def test_overwrite_inject(self):
        class Mandy(InjectMixin):
            number = Int32(displayedName="whatever", minExc=7,
                           accessMode=AccessMode.READONLY,
                           allowedStates={State.ON}, tags=set(),
                           unitSymbol=Unit.METER,
                           metricPrefixSymbol=MetricPrefix.MILLI,
                           options=[8, 9, 10])

            numberEnum = Int32(displayedName="EnumAccess",
                               defaultValue=AccessLevel.OPERATOR,
                               enum=AccessLevel)

            @Slot(displayedName="MandyRandy", allowedStates=[State.INIT])
            def randyMandy(self):
                pass

            deviceId = None

            def _register_slots(self):
                pass

            def _notifyNewSchema(self):
                pass

            def signalChanged(self, deviceId, hsh):
                pass

        mandy = Mandy()
        setter_before_inject = mandy.__class__.number.setter
        mandy.__class__.number = Overwrite(
            minExc=3, allowedStates={State.OFF},
            accessMode=AccessMode.INITONLY,
            unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MEGA,
            tags={"naughty"}, options=[6, 4])

        mandy.__class__.numberEnum = Overwrite(defaultValue=AccessLevel.ADMIN,
                                               options=[AccessLevel.ADMIN])

        mandy.__class__.randyMandy = Overwrite(
            displayedName="NoMandy", allowedStates=[State.ON]
        )
        run_coro(mandy.publishInjectedParameters())
        setter_after_inject = mandy.__class__.number.setter
        self.assertEqual(mandy.number.descriptor.key, "number")
        self.assertEqual(Mandy.number.minExc, 7)
        self.assertEqual(mandy.number.descriptor.minExc, 3)
        self.assertEqual(Mandy.number.displayedName, "whatever")
        self.assertEqual(mandy.number.descriptor.displayedName, "whatever")
        self.assertEqual(Mandy.number.allowedStates, {State.ON})
        self.assertEqual(mandy.number.descriptor.allowedStates, {State.OFF})
        self.assertEqual(Mandy.number.tags, set())
        self.assertEqual(mandy.number.descriptor.tags, {"naughty"})
        self.assertIs(Mandy.number.accessMode, AccessMode.READONLY)
        self.assertIs(mandy.number.descriptor.accessMode, AccessMode.INITONLY)
        self.assertIs(Mandy.number.unitSymbol, Unit.METER)
        self.assertIs(mandy.number.descriptor.unitSymbol, Unit.SECOND)
        self.assertEqual(Mandy.number.units, unit.millimeter)
        self.assertEqual(mandy.number.descriptor.units, unit.megasecond)
        self.assertEqual(Mandy.number.options, [8, 9, 10])
        self.assertEqual(mandy.number.descriptor.options, [6, 4])
        self.assertEqual(mandy.numberEnum.descriptor.options,
                         [AccessLevel.ADMIN])
        self.assertEqual(mandy.numberEnum.descriptor.defaultValue,
                         AccessLevel.ADMIN)
        self.assertIs(mandy.randyMandy.descriptor.displayedName, "NoMandy")
        self.assertEqual(mandy.randyMandy.descriptor.allowedStates,
                         {State.ON})
        self.assertEqual(setter_before_inject, setter_after_inject)

    def test_slot(self):
        class A(Configurable):
            @Slot(requiredAccessLevel=AccessLevel.EXPERT)
            def s(self):
                """this is a comment"""
                return 7

            @Slot()
            @asyncio.coroutine
            def c(self):
                """another comment"""
                return 8

        a = A({})
        self.assertEqual(a.s.__doc__, "this is a comment")
        self.assertEqual(a.c.__doc__, "another comment")
        self.assertIs(A.s.requiredAccessLevel, AccessLevel.EXPERT)
        self.assertIs(A.c.requiredAccessLevel, AccessLevel.USER)
        self.assertEqual(a.s(), 7)
        self.assertEqual(run_coro(a.c()), 8)
        self.assertIs(a.s.descriptor, A.s)
        self.assertIs(a.c.descriptor, A.c)
        self.assertFalse(asyncio.iscoroutinefunction(a.s))
        self.assertTrue(asyncio.iscoroutinefunction(a.c))

    def test_daqDataType(self):
        class B(Configurable):
            daqDataType = DaqDataType.TRAIN

        class A(StoreChanges):
            node = Node(B)

        a = A()
        self.assertEqual(a.node.daqDataType, DaqDataType.TRAIN)

    def test_allowedActions(self):
        class B(Configurable):
            allowedActions = ["ROI", "CROP"]

        class A(StoreChanges):
            node = Node(B)

        a = A()
        self.assertEqual(a.node.allowedActions, ["ROI", "CROP"])

    def test_daqPolicy(self):
        class MyNode(Configurable):
            string = String()

        class A(Configurable):
            a = Bool()
            b = Float()
            c = VectorFloat()
            d = Int8()
            e = VectorInt8()
            f = UInt8()
            g = VectorUInt8()
            h = Int16()
            i = VectorInt16()
            j = UInt16()
            k = VectorUInt16()
            l = Int32()
            m = VectorInt32()
            n = UInt32()
            o = VectorUInt32()
            p = Int64()
            q = VectorInt64()
            r = UInt64()
            s = VectorUInt64()
            t = Double()
            u = VectorDouble()
            v = ComplexFloat()
            w = VectorComplexFloat()
            x = ComplexDouble()
            y = VectorComplexDouble()
            z = String()
            az = VectorString()
            aa = VectorBool()

            node = Node(MyNode)

            @Slot(requiredAccessLevel=AccessLevel.EXPERT)
            def mySlot(self):
                pass

        conf = A()
        self.assertEqual(conf.a.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.b.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.c.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.e.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.f.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.g.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.h.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.i.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.j.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.k.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.l.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.m.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.n.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.o.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.p.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.q.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.r.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.s.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.t.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.u.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.v.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.w.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.x.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.y.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.z.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.az.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)
        self.assertEqual(conf.aa.descriptor.daqPolicy, DaqPolicy.UNSPECIFIED)

        # Slots have descriptor but no daqPolicy
        self.assertIsNotNone(conf.mySlot.descriptor)
        with self.assertRaises(AttributeError):
            print(conf.mySlot.descriptor.daqPolicy)

        # Nodes don't have default daqPolicy or descriptor
        self.assertIsNotNone(conf.node)
        with self.assertRaises(AttributeError):
            print(conf.node.descriptor)

    def test_deviceNode(self):
        class A(Configurable):
            node = DeviceNode()

        a = A({"node": "remote"})
        schema = a.getClassSchema()
        self.assertEqual(schema.hash['node', 'accessMode'],
                         AccessMode.INITONLY.value)
        self.assertEqual(schema.hash['node', 'assignment'],
                         Assignment.MANDATORY.value)

    def test_deviceNode_default(self):
        class A(Configurable):
            node = DeviceNode(defaultValue="remote")

        a = A()
        schema = a.getClassSchema()
        self.assertEqual(schema.hash['node', 'accessMode'],
                         AccessMode.INITONLY.value)
        self.assertEqual(schema.hash['node', 'assignment'],
                         Assignment.MANDATORY.value)
        self.assertEqual(schema.hash['node', 'defaultValue'],
                         "remote")
        # Becomes a node!
        self.assertEqual(a.node, None)

    def test_regex_string(self):

        class A(Configurable):
            d = RegexString(
                defaultValue="2",
                regex=r"(0|1|[T]rue|[F]alse)")

        # Regex does not comply with the defaultValue
        with self.assertRaises(KaraboError):
            a = A()

        class B(A):
            d = Overwrite(defaultValue="1")

        a = B()
        self.assertEqual(a.d.descriptor.defaultValue, "1")
        self.assertEqual(a.d.descriptor.classId, "RegexString")

    def test_image_with_image_data(self):
        arrayEqual = numpy.testing.assert_array_equal

        class A(Configurable):
            image = Image(
                displayedName="Image",
                data=ImageData(numpy.zeros(shape=(256, 256),
                                           dtype=numpy.uint8)))

        a = A()
        schema = a.getClassSchema()
        self.assertEqual(schema.hash['image', 'accessMode'],
                         AccessMode.READONLY.value)
        self.assertEqual(schema.hash['image', 'daqDataType'],
                         DaqDataType.TRAIN.value)
        arrayEqual(schema.hash['image.pixels.shape', 'defaultValue'],
                   numpy.array([256, 256], dtype=numpy.uint64))
        self.assertEqual(
            schema.hash['image.pixels.type', 'defaultValue'], numpy.int32(6))
        self.assertEqual(
            schema.hash['image.pixels.isBigEndian', 'defaultValue'], False)

        # An image type has a descriptor
        self.assertIsNotNone(a.image.descriptor)
        self.assertEqual(a.image.descriptor.displayedName, "Image")
        self.assertEqual(a.image.descriptor.accessMode, AccessMode.READONLY)
        self.assertEqual(a.image.descriptor.daqDataType, DaqDataType.TRAIN)

        #  Now check that we can initialize with `dtype` and `shape`
        class b(Configurable):
            image = Image(
                displayedName="Image",
                dtype=UInt8,
                shape=(256, 256))

        B = b()
        schema = B.getClassSchema()
        self.assertEqual(schema.hash['image', 'accessMode'],
                         AccessMode.READONLY.value)
        self.assertEqual(schema.hash['image', 'daqDataType'],
                         DaqDataType.TRAIN.value)
        arrayEqual(schema.hash['image.pixels.shape', 'defaultValue'],
                   numpy.array([256, 256], dtype=numpy.uint64))
        self.assertEqual(
            schema.hash['image.pixels.type', 'defaultValue'], numpy.int32(6))
        self.assertEqual(
            schema.hash['image.pixels.isBigEndian', 'defaultValue'], False)

    def test_archivePolicy(self):

        keys = ['aa', 'az']
        for c in string.ascii_lowercase:
            keys.append(c)

        class Default(Configurable):
            a = Bool()
            b = Float()
            c = VectorFloat()
            d = Int8()
            e = VectorInt8()
            f = UInt8()
            g = VectorUInt8()
            h = Int16()
            i = VectorInt16()
            j = UInt16()
            k = VectorUInt16()
            l = Int32()
            m = VectorInt32()
            n = UInt32()
            o = VectorUInt32()
            p = Int64()
            q = VectorInt64()
            r = UInt64()
            s = VectorUInt64()
            t = Double()
            u = VectorDouble()
            v = ComplexFloat()
            w = VectorComplexFloat()
            x = ComplexDouble()
            y = VectorComplexDouble()
            z = String()
            az = VectorString()
            aa = VectorBool()

        class NoArchive(Default):
            a = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            b = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            c = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            d = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            e = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            f = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            g = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            h = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            i = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            j = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            k = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            l = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            m = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            n = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            o = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            p = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            q = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            r = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            s = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            t = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            u = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            v = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            w = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            x = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            y = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            z = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            az = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            aa = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)

        a = Default()
        b = NoArchive()
        schema_a = a.getClassSchema()
        schema_b = b.getClassSchema()
        for key in keys:
            # a archives but is lazily built
            self.assertFalse(
                'archivePolicy' in schema_a.hash[key, ...])
            prop_a = getattr(a, key)
            self.assertIsNone(prop_a.descriptor.archivePolicy)
            # b does not archive
            self.assertEqual(schema_b.hash[key, 'archivePolicy'],
                             ArchivePolicy.NO_ARCHIVING.value)
            prop_b = getattr(b, key)
            self.assertEqual(prop_b.descriptor.archivePolicy,
                             ArchivePolicy.NO_ARCHIVING, key)

        i = Int32(archivePolicy=2)
        self.assertEqual(i.archivePolicy,
                         ArchivePolicy.EVERY_1S)
        with self.assertRaises(ValueError):
            Int32(archivePolicy=42)


if __name__ == "__main__":
    main()
