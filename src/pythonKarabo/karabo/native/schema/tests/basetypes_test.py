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
import operator
from enum import Enum
from itertools import product
from unittest import TestCase, main

import numpy
import pint

from karabo.native import (
    BoolValue, Encoding, EnumValue, Float, Hash, HashList, ImageData, Int32,
    KaraboValue, MetricPrefix, NoneValue, QuantityValue, StringValue,
    TableValue, Timestamp, Unit, VectorCharValue, VectorFloat,
    VectorStringValue, encodeBinary, isSet, isStringSet, unit_registry as unit,
    wrap)

TEST_REPR_HL = "\
+-----------+--------------------------------+\n\
|   integer | object                         |\n\
+===========+================================+\n\
|         3 | asdf                           |\n\
+-----------+--------------------------------+\n\
|         2 | [0, 1, 2, 3, 4, 5, 6, 7, 8, 9] |\n\
+-----------+--------------------------------+\n\
|         4 | b'fdas'                        |\n\
+-----------+--------------------------------+\n\
|         5 | bytearray(b'')                 |\n\
+-----------+--------------------------------+"


class Tests(TestCase):
    t1 = Timestamp("2009-09-01")
    t2 = Timestamp("1978-04-07")

    def test_str(self):
        s = StringValue("abc", descriptor=5, timestamp=3)
        self.assertEqual(s, "abc")
        self.assertEqual(("abc" == s).timestamp, 3)
        self.assertEqual(s.descriptor, 5)
        self.assertEqual(s.timestamp, 3)
        self.assertEqual(s.value, s)
        self.assertIs(type(s.value), str)
        self.assertEqual(next(iter(s)), "a")
        self.assertIs(next(iter(s)).timestamp, 3)

        s2 = StringValue(s)
        self.assertTrue(s == s2)
        self.assertEqual(s, s2)

        b = VectorCharValue(b"ase", descriptor=7, timestamp=2)
        self.assertEqual(b, b"ase")
        self.assertNotEqual(b, b"ddsa")
        self.assertTrue(b == b"ase")
        self.assertTrue(b"ase" == b)
        self.assertFalse(b == "ase")
        self.assertFalse("ase" == b)
        self.assertEqual(b.descriptor, 7)
        self.assertEqual(b.timestamp, 2)
        self.assertEqual(b.value, b)
        self.assertIs(type(b.value), bytes)
        self.assertEqual(next(iter(b)), 97)
        self.assertEqual(next(iter(b)).timestamp, 2)

        b2 = VectorCharValue(b)
        self.assertEqual(b, b2)

        a = {s: 3, b: 4}
        self.assertEqual(a["abc"], 3)
        self.assertEqual(a[b"ase"], 4)
        a = {"abc": 5, b"ase": 6}
        self.assertEqual(a[s], 5)
        self.assertEqual(a[b], 6)

        q = QuantityValue("3 m", timestamp=self.t1)
        s = StringValue("bla {}", timestamp=self.t2)
        self.assertEqual(s.format(q).timestamp, self.t1)
        s = StringValue("bla {blub}", timestamp=self.t2)
        self.assertEqual(s.format(blub=q).timestamp, self.t1)

        q = QuantityValue("3 m", timestamp=self.t2)
        s = StringValue("bla {}", timestamp=self.t1)
        self.assertEqual(s.format(q).timestamp, self.t1)

        s = StringValue("a,b", timestamp=self.t2)
        sep = StringValue(",", timestamp=self.t1)
        head, sep, tail = s.partition(sep)
        self.assertEqual(head.timestamp, self.t1)
        self.assertEqual(sep.timestamp, self.t1)
        self.assertEqual(tail.timestamp, self.t1)
        self.assertEqual(f"{s:5}", "a,b  ")

        ll = s.split(sep)
        self.assertEqual(ll.timestamp, self.t1)
        self.assertIsInstance(ll, VectorStringValue)

    def test_bool(self):
        t = BoolValue(True, descriptor=7, timestamp=22)
        self.assertTrue(t)
        self.assertEqual(t.descriptor, 7)
        self.assertEqual(t.timestamp, 22)
        self.assertEqual(str(t), "True")
        self.assertEqual(repr(t), "True")
        self.assertEqual(int(t), 1)
        self.assertEqual(float(t), 1.0)

        f = BoolValue(False, descriptor=3, timestamp=33)
        self.assertFalse(f)
        self.assertEqual(f.descriptor, 3)
        self.assertEqual(f.timestamp, 33)
        self.assertEqual(str(f), "False")
        self.assertEqual(repr(f), "False")
        self.assertEqual(int(f), 0)
        self.assertEqual(float(f), 0.0)

        # BoolValue isn't a bool, assure we can still serialize
        encodeBinary(Hash("t", t, "f", f))

        c = BoolValue(f)
        self.assertFalse(c)
        self.assertEqual(c.timestamp, 33)

        a = {f: 2, t: 3}
        self.assertEqual(a[False], 2)
        self.assertEqual(a[True], 3)
        a = {False: 2, True: 3}
        self.assertEqual(a[f], 2)
        self.assertEqual(a[t], 3)

        with self.assertRaises(TypeError) as exc:
            for _ in t:
                pass
            self.assertIn('BoolValue is not iterable', exc.value)

    def test_none(self):
        n = NoneValue(descriptor=7, timestamp=22)
        self.assertFalse(n)
        self.assertEqual(n, None)
        self.assertFalse(isSet(n))
        self.assertEqual(n.descriptor, 7)
        self.assertEqual(n.timestamp, 22)
        self.assertEqual(str(n), "None")
        self.assertEqual(repr(n), "None")

        encodeBinary(Hash("n", n))

        nn = NoneValue(n)
        self.assertEqual(nn.timestamp, 22)

        a = {n: 3}
        self.assertEqual(a[None], 3)
        a = {None: 2}
        self.assertEqual(a[n], 2)

        with self.assertRaises(TypeError) as exc:
            for _ in n:
                pass
            self.assertIn('NoneValue is not iterable', exc.value)

    def test_isset(self):
        self.assertTrue(isSet(3))
        self.assertTrue(isSet(BoolValue(True)))
        self.assertFalse(isSet(NoneValue()))
        self.assertFalse(isSet(None))

    def test_isstringset(self):
        self.assertTrue(isStringSet("Yes"))
        self.assertTrue(isStringSet(StringValue("Jo")))
        self.assertFalse(isStringSet(NoneValue()))
        self.assertFalse(isStringSet(None))
        self.assertFalse(isStringSet(StringValue("")))
        self.assertFalse(isStringSet([]))
        self.assertFalse(isStringSet([1, 2]))

    def test_enum(self):
        class E(Enum):
            a = 3
            b = 5

        class F(Enum):
            a = 3

        class Descriptor:
            enum = E

        d = Descriptor()
        e = EnumValue(E.a, descriptor=d, timestamp=self.t1)
        f = EnumValue(E.b, descriptor=d, timestamp=self.t2)
        self.assertEqual(e, E.a)
        self.assertNotEqual(e, E.b)
        self.assertEqual(e.descriptor, d)
        self.assertNotEqual(e, F.a)
        self.assertEqual(e.timestamp, self.t1)
        self.assertEqual((e == f).timestamp, self.t1)
        self.assertEqual(str(e), "E.a")
        self.assertEqual(repr(e), "<E.a: 3>")
        self.assertEqual(e.value, 3)
        self.assertEqual(f.value, 5)

        with self.assertRaises(TypeError):
            e = EnumValue(F.a, d)

        c = EnumValue(e)
        self.assertEqual(c, e)
        self.assertEqual(c, E.a)
        self.assertEqual(c.timestamp, self.t1)

        a = {e: 3}
        self.assertEqual(a[E.a], 3)
        a = {E.a: 4}
        self.assertEqual(a[e], 4)

    def test_enum_str(self):
        class E(Enum):
            a = "bla"
            b = "blub"

        class F(Enum):
            a = "ham"

        class Descriptor:
            enum = E

        d = Descriptor()
        e = EnumValue(E.a, descriptor=d, timestamp=self.t1)
        f = EnumValue(E.b, descriptor=d, timestamp=self.t2)
        self.assertEqual(e, E.a)
        self.assertNotEqual(e, E.b)
        self.assertEqual(e.descriptor, d)
        self.assertNotEqual(e, F.a)
        self.assertEqual(e.timestamp, self.t1)
        self.assertEqual((e == f).timestamp, self.t1)
        self.assertEqual(str(e), "E.a")
        self.assertEqual(repr(e), "<E.a: 'bla'>")
        self.assertEqual(e.value, "bla")
        self.assertEqual(f.value, "blub")

        with self.assertRaises(TypeError):
            e = EnumValue(F.a, d)

        c = EnumValue(e)
        self.assertEqual(c, e)
        self.assertEqual(c, E.a)
        self.assertEqual(c.timestamp, self.t1)

        a = {e: 3}
        self.assertEqual(a[E.a], 3)
        a = {E.a: 4}
        self.assertEqual(a[e], 4)

    def test_stringlist(self):
        vecs = VectorStringValue(["abc", "b", "c"],
                                 descriptor=3, timestamp=self.t2)
        self.assertEqual(vecs, ["abc", "b", "c"])
        self.assertEqual(vecs[1], "b")
        self.assertEqual(vecs[1].timestamp, self.t2)
        self.assertEqual(vecs.descriptor, 3)
        self.assertEqual(vecs.timestamp, self.t2)
        self.assertEqual(next(iter(vecs)), "abc")
        self.assertIs(next(iter(vecs)).timestamp, self.t2)
        self.assertIn("b", set(vecs))
        self.assertNotIn("k", set(vecs))

        for s in vecs:
            self.assertEqual(s.timestamp, self.t2)

        self.assertEqual((3 * vecs).timestamp, self.t2)

        c = VectorStringValue(vecs)
        self.assertEqual(c, vecs)
        self.assertEqual(c.timestamp, self.t2)

        c = VectorStringValue(s for s in vecs)
        self.assertEqual(c, vecs)

    def test_table(self):
        dtype = numpy.dtype([("integer", "i"), ("object", "O")])
        units = {"integer": (Unit.METER, MetricPrefix.MILLI),
                 "object": (Unit.GRAM, MetricPrefix.KILO)}
        t = TableValue(numpy.array([], dtype=dtype), units)
        self.assertEqual(len(t), 0)

        t = TableValue(numpy.array([(3, "asdf"), (2, numpy.arange(10)),
                                    (4, b"fdas"), (5, bytearray())],
                                   dtype=dtype), units, timestamp=self.t2)
        self.assertEqual(t.timestamp, self.t2)
        self.assertEqual(t["integer"][0], 3 * unit.millimeter)
        self.assertEqual(t["integer"][0].timestamp, self.t2)
        self.assertEqual((2 * t["integer"])[1], 4 * unit.millimeter)
        self.assertEqual((2 * t["integer"]).timestamp, self.t2)
        self.assertEqual(t["object"][0], "asdf")
        self.assertEqual(t["object"][0].timestamp, self.t2)
        self.assertEqual(t["object"][1][3], 3 * unit.kilogram)
        self.assertEqual(t["object"][1].timestamp, self.t2)
        self.assertEqual(t[0]["integer"], 3 * unit.millimeter)
        self.assertEqual(t[0]["integer"].timestamp, self.t2)
        self.assertEqual(t[0]["object"], "asdf")
        self.assertEqual(t[0]["object"].timestamp, self.t2)
        self.assertEqual(t[1]["object"][3], 3 * unit.kilogram)
        self.assertEqual(t[1]["object"].timestamp, self.t2)
        self.assertEqual(t[2]["object"], b"fdas")
        self.assertEqual(t[2]["object"].timestamp, self.t2)
        self.assertEqual(t[3]["object"], bytearray())
        self.assertEqual(t[3]["object"].timestamp, self.t2)

        self.assertEqual(next(iter(t))["integer"], 3 * unit.millimeter)
        self.assertEqual(next(iter(t["integer"])), 3 * unit.millimeter)
        self.assertEqual(next(iter(t)).timestamp, self.t2)

        hl = t.to_hashlist()
        self.assertEqual(repr(hl), TEST_REPR_HL)
        h = hl[0]
        self.assertEqual(len(h), 2)
        self.assertEqual(h["integer"], 3)
        self.assertEqual(h["object"], "asdf")

        h = hl[1]
        self.assertEqual(h["integer"], 2)
        numpy.testing.assert_array_equal(
            h["object"], numpy.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]))
        self.assertIsInstance(hl, HashList)

        # filter rows
        indexes = t.where("integer", 2)
        self.assertEqual(indexes, [1])

        rows = t.where_value("integer", 2)
        self.assertIsInstance(rows, TableValue)
        row_hl = rows.to_hashlist()
        self.assertEqual(len(row_hl), 1)

        # Now with operator
        row_hl = t.where_value("integer", 2, operator.lt).to_hashlist()
        self.assertEqual(len(row_hl), 0)

        row_hl = t.where_value("integer", 3, operator.gt).to_hashlist()
        self.assertEqual(len(row_hl), 2)
        self.assertEqual(row_hl[0]["integer"], 4)
        self.assertEqual(row_hl[1]["integer"], 5)
        self.assertEqual(row_hl[1]["object"], bytearray())

        def cond(a, b):
            return a - 1 == b

        row_hl = t.where_value("integer", 3, cond).to_hashlist()
        self.assertEqual(len(row_hl), 1)
        self.assertEqual(row_hl[0]["integer"], 4)

        # Empty table
        t = TableValue(numpy.array([], dtype=dtype), units)
        hl = t.to_hashlist()
        self.assertEqual(len(hl), 0)
        self.assertIsInstance(hl, HashList)

        # Iterate hashes
        t = TableValue(numpy.array([(3, []), (2, []),
                                    (4, []), (5, [])],
                                   dtype=dtype), units, timestamp=self.t2)
        # Iterate over list of hashes and change the integer
        for h in t.iter_hashes():
            self.assertIsInstance(h, Hash)
            self.assertIn("object", h)
            self.assertIn("integer", h)
            h["integer"] = 25
            # Change mutable object
            h["object"].append(25)

        # Check that the integer hasn't been changed
        for h in t.iter_hashes():
            self.assertNotEqual(h["integer"], 25)
            self.assertEqual(h["object"], [25])

    def test_unit(self):
        for u, p in product(Unit, MetricPrefix):
            if u is not Unit.NOT_ASSIGNED:
                QuantityValue(1, u, p)
        QuantityValue(1, Unit.NOT_ASSIGNED)

    def test_unit_descriptor(self):
        d1 = Int32(unitSymbol=Unit.METER)
        Int32(unitSymbol=Unit.SECOND)

        a = QuantityValue("1 m", descriptor=d1, timestamp=9)
        self.assertEqual(a.magnitude, 1)
        self.assertEqual(a.descriptor, d1)
        self.assertEqual(a.timestamp, 9)

        s = StringValue("1 m", timestamp=22)
        b = QuantityValue(s)
        self.assertEqual(a, b)
        self.assertEqual(b.timestamp, 22)

        c = QuantityValue(a)
        self.assertEqual(c.magnitude, 1)
        self.assertEqual(c.timestamp, 9)

        with self.assertRaises(pint.DimensionalityError):
            QuantityValue("1 s", descriptor=d1)

    def test_nounit(self):
        d = Float()

        a = QuantityValue(3.5, descriptor=d, timestamp=10)
        self.assertEqual(a.magnitude, 3.5)
        self.assertFalse(a.dimensionality)
        self.assertEqual(a.descriptor, d)
        self.assertEqual(a.timestamp, 10)

        b = QuantityValue(a, descriptor=d)
        self.assertNotIsInstance(b.magnitude, QuantityValue)
        self.assertEqual(b.magnitude, 3.5)
        self.assertFalse(b.dimensionality)
        self.assertEqual(b.timestamp, 10)

    def test_i_operators(self):
        """Using i operator with constants erases the timestamp"""
        d = Float()

        a = QuantityValue(3.5, descriptor=d, timestamp=10)
        self.assertEqual(a.magnitude, 3.5)
        self.assertEqual(a.timestamp, 10)

        b = QuantityValue(a, descriptor=d)
        self.assertEqual(b.magnitude, 3.5)
        self.assertEqual(b.timestamp, 10)

        # 1.1 None timestamp on __iadd__ with constant
        b += 20.0
        self.assertEqual(b.magnitude, 23.5)
        self.assertIsNone(b.timestamp)

        # 1.2 take newest timestamp with both quantity values
        b.timestamp = 20
        a += b
        self.assertEqual(a.magnitude, 27.0)
        self.assertEqual(a.timestamp, 20)

        # 2.1 None timestamp on __isub__ with constant
        b = QuantityValue(27.0, descriptor=d)
        b.timestamp = 20
        b -= 17.0
        self.assertEqual(b.magnitude, 10.0)
        self.assertIsNone(b.timestamp)

        # 2.2 take newest timestamp with both quantity values
        b.timestamp = 100
        a = QuantityValue(3.5, descriptor=d, timestamp=10)
        b -= a
        self.assertEqual(b.magnitude, 6.5)
        self.assertEqual(b.timestamp, 100)

        # 3.1 None timestamp on __imul__ with constant
        b = QuantityValue(10.0, descriptor=d)
        b.timestamp = 100
        b *= 2
        self.assertEqual(b.magnitude, 20.0)
        self.assertIsNone(b.timestamp)

        # 3.2 take newest timestamp with both quantity values
        a = QuantityValue(5, descriptor=d, timestamp=10)
        b.timestamp = 20
        b *= a
        self.assertEqual(b.magnitude, 100.0)
        self.assertEqual(b.timestamp, 20)

        # 4.1 None timestamp on __itruediv__ with constant
        b = QuantityValue(20, descriptor=d)
        b.timestamp = 20
        b /= 2
        self.assertEqual(b.magnitude, 10.0)
        self.assertIsNone(b.timestamp)

        # 4.2 take newest timestamp with both quantity values
        a = QuantityValue(5, descriptor=d, timestamp=10)
        b.timestamp = 20
        b /= a
        self.assertEqual(b.magnitude, 2.0)
        self.assertEqual(b.timestamp, 20)

        # 5.1 None timestamp on __ifloordiv__ with constant
        b = QuantityValue(20, descriptor=d)
        b.timestamp = 20
        b //= 2
        self.assertEqual(b.magnitude, 10)
        self.assertIsNone(b.timestamp)

        # 5.2 take newest timestamp with both quantity values
        a = QuantityValue(5, descriptor=d, timestamp=10)
        b.timestamp = 20
        b //= a
        self.assertEqual(b.magnitude, 2)
        self.assertEqual(b.timestamp, 20)

        # 6.1 the stacking case
        a = QuantityValue(10, descriptor=d)
        a.timestamp = 3
        b = QuantityValue(20, descriptor=d)
        b.timestamp = 1

        b += 5 + a
        self.assertEqual(b.magnitude, 35)
        self.assertEqual(b.timestamp, 3)

        # 6.2
        a = QuantityValue(10, descriptor=d)
        a.timestamp = 3
        b = QuantityValue(20, descriptor=d)
        b.timestamp = 1

        b += (5 + a) * 2
        self.assertEqual(b.magnitude, 50)
        self.assertEqual(b.timestamp, 3)

    def test_int(self):
        a = QuantityValue(3)
        # range calls a.__index__
        self.assertEqual(list(range(a)), [0, 1, 2])

        a = QuantityValue(3.5)
        with self.assertRaises(AttributeError):
            range(a)

    def test_displayType(self):
        descriptor = Int32(displayType="hex")
        a = QuantityValue(123, descriptor=descriptor)
        self.assertEqual(str(a), "0x7b")
        descriptor.displayType = "oct"
        self.assertEqual(str(a), "0o173")
        descriptor.displayType = "bin"
        self.assertEqual(str(a), "0b1111011")
        descriptor.displayType = "bin|0:a,1:b,2:c,3:d"
        self.assertEqual(str(a), "{a|b|d}")

    def test_special(self):
        vps = QuantityValue(1, Unit.VOLT_PER_SECOND)
        aps = QuantityValue(1, Unit.AMPERE_PER_SECOND)
        percent = QuantityValue(1, Unit.PERCENT)
        mps = QuantityValue(1, Unit.METER_PER_SECOND)
        pixel = QuantityValue(1, Unit.PIXEL)
        kat = QuantityValue(1, Unit.KATAL)
        degC = QuantityValue(1, Unit.DEGREE_CELSIUS)
        eV = QuantityValue(1, Unit.ELECTRONVOLT)
        number = QuantityValue(1, Unit.NUMBER)

        self.assertEqual(vps / aps, QuantityValue("1 V/A"))
        self.assertNotEqual(10 * percent, 0.1 * pixel)
        self.assertEqual(1 * degC + QuantityValue("1 K"), 2 * degC)
        self.assertEqual(2 * number, 2)
        self.assertEqual(pixel, 1 * unit.pixel)

        # check correct dimensionality:
        eV + QuantityValue("1 J")
        kat / mps + QuantityValue("1 mol / m")

    def test_wrap(self):
        w = wrap(True)
        self.assertTrue(w)
        self.assertEqual(w.descriptor, None)

        w = wrap(5)
        self.assertEqual(w, 5)
        self.assertEqual(w.magnitude, 5)
        self.assertEqual(w.descriptor, None)

        w = wrap(5.5)
        self.assertEqual(w, 5.5)
        self.assertEqual(w.magnitude, 5.5)
        self.assertEqual(w.descriptor, None)

        w = wrap(numpy.arange(10))
        self.assertEqual(w[3], 3)
        self.assertEqual(w.descriptor, None)

        w = wrap("hallo")
        self.assertEqual(w, "hallo")
        self.assertEqual(w.descriptor, None)

        w = wrap(b"hallo")
        self.assertEqual(w, b"hallo")
        self.assertEqual(w.descriptor, None)

        w = wrap([])
        self.assertEqual(w, [])
        self.assertEqual(w.descriptor, None)

        w = wrap(["bla"])
        self.assertEqual(w, ["bla"])
        self.assertEqual(w.descriptor, None)

    def test_timestamp(self):
        a = QuantityValue("1 m", timestamp=self.t1)
        b = QuantityValue("2 m", timestamp=self.t2)
        self.assertEqual((a + b).timestamp, self.t1)
        self.assertEqual((a * 3).timestamp, self.t1)
        self.assertEqual((3 * b).timestamp, self.t2)
        self.assertEqual((a == b).timestamp, self.t1)
        self.assertEqual(numpy.sin(a / b).timestamp, self.t1)

        a = QuantityValue(numpy.arange(10), "m", timestamp=self.t1)
        self.assertEqual(a[3].timestamp, self.t1)
        self.assertEqual(a[3:5].timestamp, self.t1)
        self.assertEqual(a.T.timestamp, self.t1)
        self.assertEqual(a.sum().timestamp, self.t1)

        a = QuantityValue(numpy.arange(10), "m", timestamp=self.t1)
        b = a.reshape(2, 5)
        c = QuantityValue(numpy.arange(5), "radian", timestamp=self.t2)
        d = QuantityValue(numpy.arange(2, 7), "radian", timestamp=self.t1)
        self.assertEqual(b.timestamp, self.t1)
        self.assertEqual(b.shape, (2, 5))
        self.assertEqual(b.ndim, 2)
        self.assertEqual(b.size, 10)
        self.assertEqual(b.dtype, int)
        self.assertEqual(numpy.sin(c).timestamp, self.t2)
        self.assertEqual(numpy.arctan2(d, c).timestamp, self.t1)
        self.assertEqual(numpy.arctan2(c, d).timestamp, self.t1)

        # ValueError: operands could not be broadcast together with shapes
        # (10,) (2,5)
        with self.assertRaises(ValueError):
            c = a + 1j * b

    def test_vector(self):
        v = QuantityValue([2, 3, 4], "m", timestamp=self.t1)
        self.assertEqual(next(iter(v)), 2 * unit.m)
        self.assertIs(next(iter(v)).timestamp, self.t1)

        self.assertEqual(str(v), '[2 3 4] m')
        f = QuantityValue([2., 3.3, 4.44], "m", timestamp=self.t1)
        self.assertEqual(str(f), '[2.0 3.3 4.44] m')

        a = Float(absoluteError=1e-8, relativeError=1e-12)
        v = VectorFloat(minSize=1, maxSize=3, unitSymbol=Unit.METER)
        F = v.toKaraboValue([a.toKaraboValue(2.),
                             a.toKaraboValue(3.4),
                             a.toKaraboValue(4.44)])
        self.assertEqual(str(F), '[2.0 3.4 4.44] m')

        # very small values:
        F = v.toKaraboValue([a.toKaraboValue(1e-10),
                             a.toKaraboValue(2e-20),
                             a.toKaraboValue(-4e-30)])
        # XXX: ideally we should have zeros for very small values here:
        self.assertEqual(str(F), '[1e-10 2e-20 -4e-30] m')

    def test_hash_equal(self):
        a = Hash('v', numpy.array([1, 2, 3]))
        b = Hash('v', numpy.array([1, 2, 3]))
        self.assertEqual(a, b)

        a = Hash('v', (4, 5, 6))
        b = Hash('v', (4, 5, 6))
        self.assertEqual(a, b)

    def test_mean(self):
        a = QuantityValue(3, "m", timestamp=self.t1)
        b = QuantityValue(1000, "mm", timestamp=self.t2)
        # Needs unit less values

        if pint.__version__ > "0.22":
            exc = pint.DimensionalityError
        else:
            exc = ValueError
        with self.assertRaises(exc):
            numpy.mean([a, b])

    def test_mean_no_dim(self):
        a = QuantityValue(3, timestamp=self.t1)
        b = QuantityValue(1000, timestamp=self.t2)
        # unit less values works
        m = numpy.mean([a, b])
        self.assertEqual(m, 501.5)
        # We have a float now, no timestamp
        self.assertIsInstance(m, float)

    def test_mean_array(self):
        a = QuantityValue(numpy.array([1, 2, 3, 4]), timestamp=self.t1)
        m = numpy.mean(a)
        self.assertEqual(m, 2.5 * unit.dimensionless)
        self.assertEqual(m.timestamp, self.t1)

        b = QuantityValue(numpy.array([1, 2, 3, 4]), "m",
                          timestamp=self.t1)
        m = numpy.mean(b)
        self.assertEqual(m, 2.5 * unit.meter)
        self.assertEqual(m.timestamp, self.t1)

    def test_numpy_std(self):
        a = QuantityValue(3, "m", timestamp=self.t1)
        b = QuantityValue(1000, "mm", timestamp=self.t2)
        # Needs unit less values
        if pint.__version__ > "0.22":
            exc = pint.DimensionalityError
        else:
            exc = ValueError
        with self.assertRaises(exc):
            numpy.std([a, b])

    def test_numpy_std_nodim(self):
        a = QuantityValue(3, timestamp=self.t1)
        b = QuantityValue(1000, timestamp=self.t2)
        m = numpy.std([a, b])
        self.assertEqual(m, 498.5)
        self.assertIsInstance(m, float)

    def test_timeout(self):
        time = QuantityValue(200, Unit.SECOND,
                             metricPrefix=MetricPrefix.MILLI)
        self.assertEqual(time, 200 * unit.millisecond)
        time /= unit.second
        self.assertEqual(time, 0.2)

    def test_image_data(self):
        # Initialize must take an array
        with self.assertRaises(TypeError):
            image = ImageData()

        arrayEqual = numpy.testing.assert_equal

        image = ImageData(numpy.zeros(shape=(1000, 1000), dtype=numpy.uint32))

        arrayEqual(image.value, numpy.zeros(shape=(1000, 1000),
                                            dtype=numpy.uint32))
        self.assertEqual(image.dtype, numpy.uint32)

        arrayEqual(image.dims, numpy.array([1000, 1000], dtype=numpy.uint64))
        self.assertEqual(image.dims.dtype, numpy.uint64)
        self.assertEqual(image.shape, (1000, 1000))

        arrayEqual(image.binning, numpy.array([1, 1], dtype=numpy.uint64))
        self.assertEqual(image.binning.dtype, numpy.uint64)

        self.assertEqual(image.encoding, 0)
        self.assertEqual(image.encoding.dtype, numpy.int32)
        self.assertEqual(image.flipX, False)
        self.assertEqual(image.flipY, False)
        self.assertEqual(image.rotation, 0)
        self.assertEqual(image.rotation.dtype, numpy.int32)

        arrayEqual(image.roiOffsets, numpy.array([0, 0], dtype=numpy.uint64))
        self.assertEqual(image.roiOffsets.dtype, numpy.uint64)
        self.assertEqual(image.bitsPerPixel, 32)

        image = ImageData(numpy.zeros(shape=(1000, 1000, 2),
                                      dtype=numpy.uint64),
                          encoding=Encoding.RGB,
                          flipX=True)

        arrayEqual(image.value, numpy.zeros(shape=(1000, 1000, 2),
                                            dtype=numpy.uint64))
        self.assertEqual(image.dtype, numpy.uint64)

        self.assertEqual(image.shape, (1000, 1000, 2))

        arrayEqual(image.dims, numpy.array([1000, 1000, 2],
                                           dtype=numpy.uint64))
        self.assertEqual(image.dims.dtype, numpy.uint64)

        arrayEqual(image.binning, numpy.array([1, 1, 1], dtype=numpy.uint64))
        self.assertEqual(image.binning.dtype, numpy.uint64)

        self.assertEqual(image.encoding, Encoding.RGB.value)
        self.assertEqual(image.encoding.dtype, numpy.int32)
        self.assertEqual(image.flipX, True)
        self.assertEqual(image.flipY, False)
        self.assertEqual(image.rotation, 0)
        self.assertEqual(image.rotation.dtype, numpy.int32)

        arrayEqual(image.roiOffsets, numpy.array([0, 0, 0],
                                                 dtype=numpy.uint64))
        self.assertEqual(image.roiOffsets.dtype, numpy.uint64)
        self.assertEqual(image.bitsPerPixel, 128)

    def test_round_trip_karabo_value(self):
        string = StringValue("abc", descriptor=5, timestamp=3)
        vec_char = VectorCharValue(b"ase", descriptor=7, timestamp=2)
        quantity = QuantityValue("3 m", timestamp=self.t1)
        bool_value = BoolValue(True, descriptor=7, timestamp=22)
        vector_string = VectorStringValue(["abc", "b", "c"],
                                          descriptor=3, timestamp=self.t2)
        none = NoneValue(descriptor=7, timestamp=22)
        dtype = numpy.dtype([("integer", "i"), ("object", "O")])
        table = TableValue(numpy.array([], dtype=dtype), units=None)

        class E(Enum):
            a = 3
            b = 5

        enum_value = EnumValue(E.a, descriptor=None, timestamp=self.t1)
        image = ImageData(numpy.zeros(shape=(1000, 1000), dtype=numpy.uint32))

        for value in (string, vec_char, quantity, bool_value,
                      vector_string, none, table, image, enum_value):
            value = value.value
            self.assertNotIsInstance(value, KaraboValue)


if __name__ == "__main__":
    main()
