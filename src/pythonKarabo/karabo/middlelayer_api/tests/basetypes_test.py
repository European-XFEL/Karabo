from enum import Enum
from itertools import product
from unittest import TestCase, main

import pint
import numpy

from karabo.middlelayer_api.enums import Unit, MetricPrefix
from karabo.middlelayer_api.basetypes import (
    QuantityValue, StringValue, VectorCharValue, BoolValue, EnumValue,
    VectorStringValue, wrap)
from karabo.middlelayer_api.hash import Int32, Float
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

        s2 = StringValue(s)
        self.assertTrue(s == s2)
        self.assertEqual(s, s2)

        b = VectorCharValue(b"ase", descriptor=7, timestamp=2)
        self.assertEqual(b, b"ase")
        self.assertTrue(b == b"ase")
        self.assertTrue(b"ase" == b)
        self.assertFalse(b == "ase")
        self.assertFalse("ase" == b)
        self.assertEqual(b.descriptor, 7)
        self.assertEqual(b.timestamp, 2)

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

        c = BoolValue(f)
        self.assertFalse(c)
        self.assertEqual(c.timestamp, 33)

        a = {f: 2, t: 3}
        self.assertEqual(a[False], 2)
        self.assertEqual(a[True], 3)
        a = {False: 2, True: 3}
        self.assertEqual(a[f], 2)
        self.assertEqual(a[t], 3)

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
        self.assertEqual(str(e), str(E.a))
        self.assertEqual(repr(e), repr(E.a))
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

    def test_stringlist(self):
        l = VectorStringValue(["a", "b", "c"], descriptor=3, timestamp=self.t2)
        self.assertEqual(l, ["a", "b", "c"])
        self.assertEqual(l[1], "b")
        self.assertEqual(l[1].timestamp, self.t2)
        self.assertEqual(l.descriptor, 3)
        self.assertEqual(l.timestamp, self.t2)

        for s in l:
            self.assertEqual(s.timestamp, self.t2)

        self.assertEqual((3 * l).timestamp, self.t2)

        c = VectorStringValue(l)
        self.assertEqual(c, l)
        self.assertEqual(c.timestamp, self.t2)

        c = VectorStringValue(s for s in l)
        self.assertEqual(c, l)

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

if __name__ == "__main__":
    main()
