# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import asyncio
import inspect
import string
from types import MethodType
from unittest import TestCase, main
from zlib import adler32

import numpy
import pytest
from pint import DimensionalityError

from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, Bool, ComplexDouble,
    ComplexFloat, Configurable, DaqDataType, Double, Float, Hash, Image,
    ImageData, Int8, Int16, Int32, Int64, KaraboError, MetricPrefix, Node,
    Overwrite, QuantityValue, RegexString, Slot, String, Timestamp, TypeHash,
    TypeSchema, UInt8, UInt16, UInt32, UInt64, Unit, VectorBool, VectorChar,
    VectorComplexDouble, VectorComplexFloat, VectorDouble, VectorFloat,
    VectorHash, VectorInt8, VectorInt16, VectorInt32, VectorInt64,
    VectorRegexString, VectorString, VectorUInt8, VectorUInt16, VectorUInt32,
    VectorUInt64, decodeBinary, encodeBinary, isSet, unit_registry as unit)


class RowSchema(Configurable):
    string = String()


class FullSchema(Configurable):
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
    l = Int32()  # noqa
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
    ab = TypeHash()
    ac = TypeSchema()
    ad = VectorHash(rows=RowSchema)


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

        # mandatory vector
        class RowSchema(Configurable):
            value = Int32(defaultValue=0)

        class C(Configurable):
            value = VectorHash(
                rows=RowSchema,
                assignment=Assignment.MANDATORY,
                unitSymbol=Unit.METER)
        with pytest.raises(KaraboError):
            C()

        c = C({"value": []})
        assert c is not None

    def test_bulk_set_setter(self):
        """Test the bulk setting of a full Hash on a configurable"""

        class C(Configurable):
            cvalue = Int32(accessMode=AccessMode.READONLY,
                           minInc=-2,
                           unitSymbol=Unit.METER)

        class B(Configurable):
            bvalue = Int32(accessMode=AccessMode.READONLY,
                           unitSymbol=Unit.METER)
            node = Node(C)

        class A(Configurable):
            state = String(enum=State, options=[State.ON, State.OFF])
            value = Int32(accessMode=AccessMode.READONLY,
                          unitSymbol=Unit.METER)
            node = Node(B)

        # 1. Test the basic node setting
        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.state))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))

        h = Hash("value", 5)
        a.set(h)
        self.assertEqual(a.value, 5 * unit.meter)
        self.assertEqual(a.value.dtype, numpy.int32)
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))

        h["value"] = 22
        h["node.bvalue"] = 2
        a.set(h)
        self.assertEqual(a.value, 22 * unit.meter)
        self.assertEqual(a.node.bvalue, 2 * unit.meter)
        self.assertEqual(a.node.bvalue.dtype, numpy.int32)
        self.assertFalse(isSet(a.node.node.cvalue))

        h["node.node.cvalue"] = -1
        a.set(h)
        self.assertEqual(a.node.node.cvalue, -1 * unit.meter)
        self.assertIsNotNone(a.value.timestamp)
        self.assertIsNotNone(a.node.bvalue.timestamp)
        self.assertIsNotNone(a.node.node.cvalue.timestamp)

        # 2. Test exception case
        a = A()
        h = Hash("value", 12)
        h["node.bvalue"] = 12

        h["node.node.cvalue"] = -3
        with pytest.raises(ValueError):
            a.set(h)

        # All values are not set
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))

        # 3. Test KaraboValues in a Hash
        a = A()
        ts = Timestamp()
        h = Hash("value", QuantityValue(5, timestamp=ts))
        with pytest.raises(DimensionalityError):
            a.set(h)
        self.assertFalse(isSet(a.value))

        h = Hash("value", QuantityValue(5, unit=Unit.METER, timestamp=ts))
        a.set(h)
        self.assertEqual(a.value, 5 * unit.meter)
        self.assertEqual(a.value.timestamp, ts)

        # 4. Test RuntimeErrors
        a = A()
        h = {"value", 5}
        with pytest.raises(RuntimeError):
            a.set(h)

        h = Hash("value", 5)
        # Not a Hash for a Node
        h["node"] = {"bvalue": 12}
        with pytest.raises(RuntimeError):
            a.set(h)

        # 5. Test a Hash with timestamp in attributes
        a = A()
        ts = Timestamp()
        h = Hash("value", 15)
        ts.toHashAttributes(h)
        a.set(h)
        self.assertEqual(a.value, 15 * unit.meter)
        self.assertEqual(a.value.timestamp, ts)

        # 5. Test a Hash with an Enum
        a = A()
        ts = Timestamp()
        h = Hash("state", State.ON)
        ts.toHashAttributes(h)
        a.set(h)
        self.assertEqual(a.state, State.ON)
        self.assertEqual(a.state.timestamp, ts)

        # 5.1 Be less strict, so enums can convert
        h = Hash("state", "OFF")
        ts.toHashAttributes(h)
        a.set(h, strict=False)
        self.assertEqual(a.state, State.OFF)

        # 6. Invalid Option for string
        a = A()
        h = Hash("state", State.RUNNING)
        with pytest.raises(ValueError):
            a.set(h)
        self.assertFalse(isSet(a.state))

        h = Hash("state", State.RUNNING.value)
        with pytest.raises(ValueError):
            a.set(h)
        self.assertFalse(isSet(a.state))

        # 7. Back to None
        a = A()
        h = Hash("value", 5)
        a.set(h)
        self.assertEqual(a.value, 5 * unit.meter)
        h = Hash("value", None)
        with pytest.raises(TypeError):
            # The check cannot compare None and Int
            a.set(h)
        self.assertEqual(a.value, 5 * unit.meter)

        # 8.1 Test the only changes for leafs
        a = A()
        h = Hash("value", 5)
        a.set(h, True)
        self.assertEqual(a.value, 5 * unit.meter)
        ts = a.value.timestamp
        self.assertIsNotNone(ts)
        run_coro(asyncio.sleep(0.01))
        a.set(h, True)
        self.assertEqual(a.value.timestamp, ts)
        run_coro(asyncio.sleep(0.01))
        a.set(h, False)
        self.assertNotEqual(a.value.timestamp, ts)

        # 8.2 Test the only changes for nodes
        a = A()
        h = Hash("value", 5)
        h["node.node.cvalue"] = -1
        a.set(h, True)
        self.assertEqual(a.node.node.cvalue, -1 * unit.meter)
        ts = a.node.node.cvalue.timestamp
        self.assertIsNotNone(ts)
        run_coro(asyncio.sleep(0.01))
        a.set(h, True)
        self.assertEqual(ts, a.node.node.cvalue.timestamp)
        run_coro(asyncio.sleep(0.01))
        a.set(h, False)
        self.assertNotEqual(ts, a.node.node.cvalue.timestamp)

    def test_bulk_set_via_setter(self):
        """Test the bulk setting of a full Hash on a configurable via setter"""

        csetter_called = False
        bsetter_called = False

        class C(Configurable):
            @Int32(
                accessMode=AccessMode.READONLY,
                minInc=-2,
                unitSymbol=Unit.METER)
            def cvalue(self, value):
                self.cvalue = value
                nonlocal csetter_called
                csetter_called = True

        class B(Configurable):
            @Int32(accessMode=AccessMode.READONLY,
                   unitSymbol=Unit.METER)
            def bvalue(self, value):
                self.bvalue = value
                nonlocal bsetter_called
                bsetter_called = True

            node = Node(C)

        class A(Configurable):
            state = String(enum=State,
                           options=[State.ON, State.OFF])

            @Double(accessMode=AccessMode.READONLY,
                    unitSymbol=Unit.METER)
            def value(self, value):
                if isSet(value):
                    self.value = value * 2
                else:
                    self.value = value
            node = Node(B)

        # 1. Test the basic node setting
        a = A()
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))

        h = Hash("value", 5)
        run_coro(a.set_setter(h))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.value.dtype, numpy.float64)
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))

        bsetter_called = False
        h["value"] = 22
        h["node.bvalue"] = 2
        run_coro(a.set_setter(h))
        self.assertEqual(a.value, 44 * unit.meter)
        self.assertEqual(a.node.bvalue, 2 * unit.meter)
        self.assertEqual(a.node.bvalue.dtype, numpy.int32)
        self.assertTrue(bsetter_called)
        self.assertFalse(isSet(a.node.node.cvalue))

        csetter_called = False
        h["node.node.cvalue"] = -1
        run_coro(a.set_setter(h))
        self.assertEqual(a.node.node.cvalue, -1 * unit.meter)
        self.assertIsNotNone(a.value.timestamp)
        self.assertIsNotNone(a.node.bvalue.timestamp)
        self.assertIsNotNone(a.node.node.cvalue.timestamp)
        self.assertTrue(csetter_called)

        # 2. Test exception case
        a = A()
        h = Hash("value", 12)
        h["node.bvalue"] = 12
        h["node.node.cvalue"] = -3

        bsetter_called = False
        csetter_called = False
        with pytest.raises(ValueError):
            run_coro(a.set_setter(h))

        # All values are not set
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))
        self.assertFalse(bsetter_called)
        self.assertFalse(csetter_called)

        # 3. Test KaraboValues in a Hash
        a = A()
        ts = Timestamp()
        h = Hash("value", QuantityValue(5, timestamp=ts))
        with pytest.raises(DimensionalityError):
            run_coro(a.set_setter(h))
        self.assertFalse(isSet(a.value))

        h = Hash("value", QuantityValue(5, unit=Unit.METER, timestamp=ts))
        run_coro(a.set_setter(h))
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.value.timestamp, ts)

        # 4. Test RuntimeErrors
        a = A()
        h = {"value", 5}
        with pytest.raises(RuntimeError):
            run_coro(a.set_setter(h))

        h = Hash("value", 5)
        # Not a Hash for a Node
        h["node"] = {"bvalue": 12}
        with pytest.raises(RuntimeError):
            run_coro(a.set_setter(h))

        # 5. Test a Hash with timestamp in attributes
        a = A()
        ts = Timestamp()
        h = Hash("value", 15)
        ts.toHashAttributes(h)
        run_coro(a.set_setter(h))
        self.assertEqual(a.value, 30 * unit.meter)
        self.assertEqual(a.value.timestamp, ts)

        # 5. Test a Hash with an Enum
        a = A()
        ts = Timestamp()
        h = Hash("state", State.ON)
        ts.toHashAttributes(h)
        run_coro(a.set_setter(h))
        self.assertEqual(a.state, State.ON)
        self.assertEqual(a.state.timestamp, ts)

        # 6. Invalid Option for string
        a = A()
        h = Hash("state", State.RUNNING)
        with pytest.raises(ValueError):
            run_coro(a.set_setter(h))
        self.assertFalse(isSet(a.state))

        h = Hash("state", State.RUNNING.value)
        with pytest.raises(ValueError):
            run_coro(a.set_setter(h))
        self.assertFalse(isSet(a.state))

        # 7. Back to None
        a = A()
        h = Hash("value", 5)
        run_coro(a.set_setter(h))
        self.assertEqual(a.value, 10 * unit.meter)

        h = Hash("value", None)
        try:
            # The double converts to nan
            run_coro(a.set_setter(h))
        except TypeError:
            self.fail("Could not set None value on floating point")

        self.assertIsNotNone(a.value.value)
        numpy.testing.assert_equal(a.value.value, numpy.nan)

        # 8.1 Test the only changes for leafs
        a = A()
        h = Hash("value", 5)
        run_coro(a.set_setter(h))
        self.assertEqual(a.value, 10 * unit.meter)
        ts = a.value.timestamp
        self.assertIsNotNone(ts)
        run_coro(asyncio.sleep(0.01))
        h = Hash("value", 10)
        run_coro(a.set_setter(h, True))
        # Wasn't changed, setter not triggered
        self.assertEqual(a.value, 10 * unit.meter)
        self.assertEqual(a.value.timestamp, ts)
        run_coro(asyncio.sleep(0.01))
        run_coro(a.set_setter(h))
        self.assertNotEqual(a.value.timestamp, ts)

        # 8.2 Test the only changes for nodes
        a = A()
        h = Hash("value", 5)
        h["node.node.cvalue"] = -1
        run_coro(a.set_setter(h, True))
        self.assertEqual(a.node.node.cvalue, -1 * unit.meter)
        ts = a.node.node.cvalue.timestamp
        self.assertIsNotNone(ts)
        run_coro(asyncio.sleep(0.01))
        run_coro(a.set_setter(h, True))
        self.assertEqual(ts, a.node.node.cvalue.timestamp)
        run_coro(asyncio.sleep(0.01))
        run_coro(a.set_setter(h, False))
        self.assertNotEqual(ts, a.node.node.cvalue.timestamp)

        # 9. Test empty hash
        h = Hash()
        a = A()
        run_coro(a.set_setter(h))
        # All values are not set
        self.assertFalse(isSet(a.value))
        self.assertFalse(isSet(a.node.bvalue))
        self.assertFalse(isSet(a.node.node.cvalue))

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
            async def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            async def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                await super().initialize(instance, value + 2 * unit.meter)

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
            async def setter(self, instance, value):
                nonlocal setter_value
                setter_value += value
                super().setter(instance, value + 1 * unit.meter)

            async def initialize(self, instance, value):
                nonlocal init_value
                init_value += value
                await super().initialize(instance, value + 2 * unit.meter)

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

        # Test initialization

        class A(Configurable):
            stringVector = VectorString(
                defaultValue=['blub'],
                minSize=2)

        with self.assertRaises(ValueError):
            A()

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

        self.assertEqual(adler32(str(a.table).encode("ascii")), 4066904189)

        # Test the comparison of table element with Hash
        h = a.configurationAsHash()
        # Check if we are equal to ourselves ...
        self.assertTrue(h.fullyEqual(a.configurationAsHash()))
        a.table.insert(2, ("C", 11, 5, ["Itchy", "Burns"]))
        self.assertFalse(h.fullyEqual(a.configurationAsHash()))

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

        # Try operations with KaraboValue's
        a = A(Hash("table", [Hash("name", "bla",
                                  "number", 5, "counter", 2,
                                  "vectorString", ["Itchy", "Burns"])]))
        # numpy.delete is not supported by pint
        # later pint version will raise a RecursionError,
        # older ones a TypeError
        with pytest.raises((RecursionError, TypeError)):
            a.table.pop(QuantityValue(numpy.int32(0)))

    def test_overwrite(self):
        class Mandy(Configurable):
            number = Int32(displayedName="whatever", minExc=7,
                           accessMode=AccessMode.READONLY,
                           allowedStates={State.ON}, tags=set(),
                           unitSymbol=Unit.METER,
                           metricPrefixSymbol=MetricPrefix.MILLI,
                           options=[8, 9, 10])

            def align(self):
                return self.options

            number.align = align

            cookies = String(allowedStates={State.HOMING},
                             displayedName="else",
                             unitSymbol=Unit.METER,
                             metricPrefixSymbol=MetricPrefix.MILLI,
                             options=["a", "b"])
            cookies.poll = True

            def parse(self):
                """A simple test of having a parse method"""
                return self.poll

            cookies.parse = MethodType(parse, cookies)

            state = String(enum=State)
            state.transformed = True

        class Brian(Mandy):
            number = Overwrite(minExc=3, allowedStates={State.OFF},
                               accessMode=AccessMode.INITONLY,
                               unitSymbol=Unit.SECOND,
                               metricPrefixSymbol=MetricPrefix.MEGA,
                               tags={"naughty"},
                               options=[6, 4],
                               extras={"align"})
            cookies = Overwrite(extras=["poll", "parse"])
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

        # Some extras ...
        # Not a method on Mandy
        assert not inspect.ismethod(Mandy.number.align)
        assert not inspect.ismethod(Brian.number.align)
        # Method in mandy
        assert inspect.ismethod(Mandy.number.setter)
        assert inspect.ismethod(Brian.number.setter)
        # Parse method defined
        self.assertIsNotNone(Brian.cookies.parse)
        assert inspect.ismethod(Mandy.cookies.parse)
        assert inspect.ismethod(Brian.cookies.parse)

        # Not transformed
        self.assertIsNotNone(Mandy.state.transformed)
        self.assertIsNone(getattr(Brian.state, "transformed", None))

        self.assertEqual(Mandy.cookies.allowedStates, {State.HOMING})
        self.assertEqual(Brian.cookies.allowedStates, {State.HOMING})
        self.assertEqual(Mandy.cookies.displayedName, "else")
        self.assertEqual(Brian.cookies.displayedName, "else")
        self.assertEqual(Brian.cookies.poll, True)

        self.assertIs(Mandy.cookies.unitSymbol, Unit.METER)
        self.assertIs(Brian.cookies.unitSymbol, Unit.METER)
        self.assertEqual(Mandy.cookies.units, unit.millimeter)
        self.assertEqual(Brian.cookies.units, unit.millimeter)
        self.assertEqual(Mandy.cookies.options, ["a", "b"])
        self.assertEqual(Brian.cookies.options, ["a", "b"])

        self.assertIsNone(Mandy.state.options)
        self.assertEqual(Brian.state.options, [State.ON, State.OFF])

    def test_slot(self):
        class A(Configurable):
            @Slot(requiredAccessLevel=AccessLevel.EXPERT)
            def s(self):
                """this is a comment"""
                return 7

            @Slot()
            async def c(self):
                """another comment"""
                return 8

        a = A({})
        self.assertEqual(a.s.__doc__, "this is a comment")
        self.assertEqual(a.c.__doc__, "another comment")
        self.assertIs(A.s.requiredAccessLevel, AccessLevel.EXPERT)
        self.assertIs(A.c.requiredAccessLevel, AccessLevel.OPERATOR)
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

    def test_assignment_internal(self):
        """Test that we can have no defaults with assignment internal"""
        class A(Configurable):
            a = Bool(assignment=Assignment.INTERNAL)
            b = Float(assignment=Assignment.INTERNAL)
            c = VectorFloat(assignment=Assignment.INTERNAL)
            d = Int8(assignment=Assignment.INTERNAL)
            e = VectorInt8(assignment=Assignment.INTERNAL)
            z = String(assignment=Assignment.INTERNAL)
            az = VectorString(assignment=Assignment.INTERNAL)
            aa = VectorBool(assignment=Assignment.INTERNAL)
            ab = VectorHash(
                rows=RowSchema,
                assignment=Assignment.INTERNAL)

        a = A()
        self.assertEqual(a.a, None)
        self.assertEqual(a.b, None)
        self.assertEqual(a.c, None)
        self.assertEqual(a.d, None)
        self.assertEqual(a.e, None)
        self.assertEqual(a.z, None)
        self.assertEqual(a.az, None)
        self.assertEqual(a.aa, None)
        self.assertEqual(a.ab, None)

    def test_regex_string(self):

        class A(Configurable):
            d = RegexString(
                defaultValue="2",
                regex=r"(0|1|[T]rue|[F]alse)")

        # Regex does not comply with the defaultValue
        with self.assertRaises(KaraboError):
            a = A()  # noqa

        class B(A):
            d = Overwrite(defaultValue="1")

        a = B()
        self.assertEqual(a.d.descriptor.defaultValue, "1")
        self.assertEqual(a.d.descriptor.classId, "RegexString")

        class C(Configurable):
            d = RegexString(
                regex=r"(0|1|[T]rue|[F]alse)")
            e = String()

        a = C()
        self.assertEqual(a.d.descriptor.defaultValue, None)
        self.assertEqual(a.d, None)
        self.assertEqual(a.d.descriptor.classId, "RegexString")
        self.assertEqual(a.e.descriptor.defaultValue, None)
        self.assertEqual(a.e, None)

    def test_vector_regex_string(self):

        class A(Configurable):
            d = VectorRegexString(
                defaultValue=["remote:output "],  # add a whitespace
                regex=r"^[A-Za-z0-9_-]{1,60}(:)[A-Za-z0-9_-]{1,60}$")

        # Regex does not comply with the defaultValue
        with self.assertRaises(KaraboError):
            a = A()  # noqa

        class B(A):
            d = Overwrite(defaultValue=["remote:output"])

        b = B()
        self.assertEqual(b.d.descriptor.defaultValue, ["remote:output"])
        self.assertEqual(b.d.descriptor.classId, "VectorRegexString")
        self.assertEqual(b.d.descriptor.displayType, "VectorRegexString")

        # No default value
        class C(Configurable):
            d = VectorRegexString(
                regex=r"^[A-Za-z0-9_-]{1,60}(:)[A-Za-z0-9_-]{1,60}$")
            e = VectorString()

        c = C()
        self.assertEqual(c.d.descriptor.classId, "VectorRegexString")
        self.assertEqual(c.d.descriptor.displayType, "VectorRegexString")
        self.assertEqual(c.d.descriptor.defaultValue, None)
        self.assertEqual(c.d, None)
        # Same behavior
        self.assertEqual(c.e.descriptor.defaultValue, None)
        self.assertEqual(c.e, None)

        # Minsize and maxsize

        class D(Configurable):
            d = VectorRegexString(
                defaultValue=["remote:output"],
                minSize=2,
                regex=r"^[A-Za-z0-9_-]{1,60}(:)[A-Za-z0-9_-]{1,60}$")

        with self.assertRaises(ValueError):
            D()

        class E(Configurable):
            d = VectorRegexString(
                defaultValue=["remote:output", "remote1:output"],
                maxSize=1,
                regex=r"^[A-Za-z0-9_-]{1,60}(:)[A-Za-z0-9_-]{1,60}$")
        with self.assertRaises(ValueError):
            E()

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

    def test_class_schema(self):
        schema = FullSchema.getClassSchema()
        shash = schema.hash

        self.assertEqual(shash["a", "valueType"], "BOOL")
        self.assertEqual(shash["b", "valueType"], "FLOAT")
        self.assertEqual(shash["c", "valueType"], "VECTOR_FLOAT")
        self.assertEqual(shash["d", "valueType"], "INT8")
        self.assertEqual(shash["e", "valueType"], "VECTOR_INT8")
        self.assertEqual(shash["f", "valueType"], "UINT8")
        self.assertEqual(shash["g", "valueType"], "VECTOR_UINT8")
        self.assertEqual(shash["h", "valueType"], "INT16")
        self.assertEqual(shash["i", "valueType"], "VECTOR_INT16")
        self.assertEqual(shash["j", "valueType"], "UINT16")
        self.assertEqual(shash["k", "valueType"], "VECTOR_UINT16")
        self.assertEqual(shash["l", "valueType"], "INT32")
        self.assertEqual(shash["m", "valueType"], "VECTOR_INT32")
        self.assertEqual(shash["n", "valueType"], "UINT32")
        self.assertEqual(shash["o", "valueType"], "VECTOR_UINT32")
        self.assertEqual(shash["p", "valueType"], "INT64")
        self.assertEqual(shash["q", "valueType"], "VECTOR_INT64")
        self.assertEqual(shash["r", "valueType"], "UINT64")
        self.assertEqual(shash["s", "valueType"], "VECTOR_UINT64")
        self.assertEqual(shash["t", "valueType"], "DOUBLE")
        self.assertEqual(shash["u", "valueType"], "VECTOR_DOUBLE")
        self.assertEqual(shash["v", "valueType"], "COMPLEX_FLOAT")
        self.assertEqual(shash["w", "valueType"], "VECTOR_COMPLEX_FLOAT")
        self.assertEqual(shash["x", "valueType"], "COMPLEX_DOUBLE")
        self.assertEqual(shash["y", "valueType"], "VECTOR_COMPLEX_DOUBLE")
        self.assertEqual(shash["z", "valueType"], "STRING")
        self.assertEqual(shash["az", "valueType"], "VECTOR_STRING")
        self.assertEqual(shash["aa", "valueType"], "VECTOR_BOOL")
        self.assertEqual(shash["ab", "valueType"], "HASH")
        self.assertEqual(shash["ac", "valueType"], "SCHEMA")
        self.assertEqual(shash["ad", "valueType"], "VECTOR_HASH")

    def test_archivePolicy(self):

        keys = ['aa', 'az', 'ab', 'ac', 'ad']
        for c in string.ascii_lowercase:
            keys.append(c)

        class NoArchive(FullSchema):
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
            l = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)  # noqa
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
            ab = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            ac = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)
            ad = Overwrite(archivePolicy=ArchivePolicy.NO_ARCHIVING)

        a = FullSchema()
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

    def test_node_schema_attrs(self):
        class Nested(Configurable):
            daqDataType = DaqDataType.TRAIN
            state = String(
                displayType="State",
                defaultValue=State.ON,
                displayedName="State",
                enum=State)
            integer = Int32(
                defaultValue=32,
                displayedName="Int")
            double = Double(
                defaultValue=25.76,
                displayedName="Double",
                allowedStates=[State.ON])

        node = Node(Nested, classId="KaraboNode")
        h, attrs = node.toSchemaAndAttrs(None, None)
        self.assertIsInstance(h, Hash)
        self.assertIsInstance(attrs, dict)
        self.assertEqual(attrs["classId"], "KaraboNode")
        self.assertEqual(attrs["daqDataType"], DaqDataType.TRAIN)
        self.assertIn("state", h)
        self.assertIn("integer", h)
        self.assertIn("double", h)

        # State dependent node schema
        h, _ = node.toSchemaAndAttrs(None, State.MOVING)
        self.assertIn("state", h)
        self.assertIn("integer", h)
        self.assertNotIn("double", h)

        instance_schema = Nested().getDeviceSchema()
        class_schema = Nested.getClassSchema()
        self.assertTrue(instance_schema.hash.fullyEqual(class_schema.hash))

        instance_schema = Nested().getDeviceSchema(State.MOVING)
        self.assertFalse(instance_schema.hash.fullyEqual(class_schema.hash))
        class_schema = Nested.getClassSchema(None, State.MOVING)
        self.assertTrue(instance_schema.hash.fullyEqual(class_schema.hash))

    def test_node_data_attrs(self):
        class Expert(Configurable):
            gear = Int32(
                defaultValue=25000,
                displayedName="Gear")

        class Motor(Configurable):
            state = String(
                displayType="State",
                defaultValue=State.ON,
                displayedName="State",
                enum=State)
            position = Double(
                defaultValue=25.76,
                displayedName="Position")
            expert = Node(Expert)

        # Trigger dataAndAttrs with configuration
        motor = Motor()
        data = motor.configurationAsHash()
        self.assertEqual(data["state"], "ON")
        # Attributes are the time information
        self.assertIn("tid", data["state", ...])
        self.assertIn("sec", data["state", ...])
        self.assertIn("frac", data["state", ...])
        self.assertEqual(data["position"], 25.76)
        self.assertEqual(data["expert.gear"], 25000)

    def test_get_root(self):
        """Test the root retrieval of the configurable"""

        class Sub(Configurable):
            integer = Int32()

        class SubNode(Configurable):
            node = Node(Sub)

        class Root(Configurable):
            node = Node(SubNode)

        root = Root()
        suspect = root.get_root()
        self.assertEqual(suspect, root)
        suspect = root.node.get_root()
        self.assertEqual(suspect, root)
        suspect = root.node.node.get_root()
        self.assertEqual(suspect, root)


if __name__ == "__main__":
    main()
