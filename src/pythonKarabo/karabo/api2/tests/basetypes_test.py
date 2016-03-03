from enum import Enum
from itertools import product
from unittest import TestCase, main

from karabo.api2.enums import Unit, MetricPrefix
from karabo.api2.basetypes import (
    QuantityValue, StringValue, VectorCharValue, BoolValue, EnumValue,
    VectorStringValue)


class Tests(TestCase):
    def test_str(self):
        s = StringValue("abc", descriptor=5)
        self.assertEqual(s, "abc")
        self.assertEqual(s.descriptor, 5)

        b = VectorCharValue(b"ase", descriptor=7)
        self.assertEqual(b, b"ase")
        self.assertEqual(b.descriptor, 7)

    def test_bool(self):
        t = BoolValue(True, descriptor=7)
        self.assertTrue(t)
        self.assertEqual(t.descriptor, 7)

        f = BoolValue(False, descriptor=3)
        self.assertFalse(f)
        self.assertEqual(f.descriptor, 3)

    def test_enum(self):
        class E(Enum):
            a = 3
            b = 5

        class F(Enum):
            a = 3

        class Descriptor:
            enum = E

        d = Descriptor()
        e = EnumValue(E.a, d)
        self.assertEqual(e, E.a)
        self.assertNotEqual(e, E.b)
        self.assertEqual(e.descriptor, d)
        self.assertNotEqual(e, F.a)

        with self.assertRaises(TypeError):
            e = EnumValue(F.a, d)

    def test_stringlist(self):
        l = VectorStringValue(["a", "b", "c"], descriptor=3)
        self.assertEqual(l, ["a", "b", "c"])
        self.assertEqual(l.descriptor, 3)

    def test_unit(self):
        for u, p in product(Unit, MetricPrefix):
            if u is not Unit.NOT_ASSIGNED:
                QuantityValue(1, u, p)
        QuantityValue(1, Unit.NOT_ASSIGNED)

    def test_unit_descriptor(self):
        a = QuantityValue("1 m", descriptor=7)
        self.assertEqual(a.magnitude, 1)
        self.assertEqual(a.descriptor, 7)

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

if __name__ == "__main__":
    main()
