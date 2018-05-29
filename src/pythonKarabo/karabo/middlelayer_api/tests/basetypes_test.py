from enum import Enum
from itertools import product
from unittest import TestCase, main

import pint
import numpy

from karabo.middlelayer import decodeBinary, encodeBinary, unit
from karabo.middlelayer_api.enums import Unit, MetricPrefix
from karabo.middlelayer_api.basetypes import (
    NoneValue, QuantityValue, isSet, StringValue, VectorCharValue, BoolValue,
    EnumValue, TableValue, VectorStringValue, wrap)
from karabo.middlelayer_api.hash import Hash, Int32, Float
from karabo.middlelayer_api.timestamp import Timestamp


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
        self.assertEqual("{:5}".format(s), "a,b  ")

        l = s.split(sep)
        self.assertEqual(l.timestamp, self.t1)
        self.assertIsInstance(l, VectorStringValue)

    def test_bool(self):
        t = BoolValue(True, descriptor=7, timestamp=22)
        self.assertTrue(t)
        self.assertEqual(t.descriptor, 7)
        self.assertEqual(t.timestamp, 22)
        self.assertEqual(str(t), "True")
        self.assertEqual(repr(t), "True")

        f = BoolValue(False, descriptor=3, timestamp=33)
        self.assertFalse(f)
        self.assertEqual(f.descriptor, 3)
        self.assertEqual(f.timestamp, 33)
        self.assertEqual(str(f), "False")
        self.assertEqual(repr(f), "False")

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

    def test_isset(self):
        self.assertTrue(isSet(3))
        self.assertTrue(isSet(BoolValue(True)))
        self.assertFalse(isSet(NoneValue()))
        self.assertFalse(isSet(None))

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
        l = VectorStringValue(["abc", "b", "c"],
                              descriptor=3, timestamp=self.t2)
        self.assertEqual(l, ["abc", "b", "c"])
        self.assertEqual(l[1], "b")
        self.assertEqual(l[1].timestamp, self.t2)
        self.assertEqual(l.descriptor, 3)
        self.assertEqual(l.timestamp, self.t2)
        self.assertEqual(next(iter(l)), "abc")
        self.assertIs(next(iter(l)).timestamp, self.t2)
        self.assertIn("b", set(l))
        self.assertNotIn("k", set(l))

        for s in l:
            self.assertEqual(s.timestamp, self.t2)

        self.assertEqual((3 * l).timestamp, self.t2)

        c = VectorStringValue(l)
        self.assertEqual(c, l)
        self.assertEqual(c.timestamp, self.t2)

        c = VectorStringValue(s for s in l)
        self.assertEqual(c, l)

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
        self.assertEqual(10 * percent, 0.1 * pixel)
        self.assertEqual(1 * degC + QuantityValue("1 K"), 2 * degC)
        self.assertEqual(2 * number, 2)

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

        c = a + 1j * b
        self.assertEqual(c.timestamp, self.t1)
        self.assertEqual(c.imag.timestamp, self.t1)
        self.assertEqual(c.real.timestamp, self.t1)

        self.assertEqual(c.to("mm").timestamp, self.t1)

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

    def test_vector(self):
        v = QuantityValue([2, 3, 4], "m", timestamp=self.t1)
        self.assertEqual(next(iter(v)), 2 * unit.m)
        self.assertIs(next(iter(v)).timestamp, self.t1)

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
        m = numpy.mean([a, b])
        self.assertEqual(m, 2 * unit.m)
        self.assertEqual(m.timestamp, self.t1)
        m = numpy.std([a, b])
        self.assertEqual(m, 1 * unit.m)
        self.assertEqual(m.timestamp, self.t1)

if __name__ == "__main__":
    main()
