from enum import Enum
from unittest import TestCase, main

from pint import DimensionalityError

from karabo.middlelayer_api import hash as hashmod
from karabo.middlelayer_api.basetypes import EnumValue, QuantityValue
from karabo.middlelayer_api.enums import Unit, MetricPrefix
from karabo.middlelayer_api.hash import Hash


class Tests(TestCase):
    def check_general(self, desc, value):
        """check things common to all values"""
        self.assertEqual(value.descriptor, desc)
        value.timestamp = 7
        c = desc.toKaraboValue(value)
        self.assertEqual(c.timestamp, 7)
        h = Hash()
        h["a"] = c.value
        h = Hash.decode(h.encode("Bin"), "Bin")
        if isinstance(desc, hashmod.NumpyVector):
            self.assertTrue((c == value).all())
            self.assertTrue((h["a"] == value.value).all())
        else:
            self.assertTrue(c == value)
            self.assertEqual(h["a"], value.value)

    def test_int_enum(self):
        class E(Enum):
            a = 3
        class F(Enum):
            a = 3
        d = hashmod.Int8(enum=E)
        v = d.toKaraboValue(E.a)
        self.assertIs(v.enum, E.a)
        self.assertEqual(v.value, 3)
        self.check_general(d, v)
        with self.assertRaises(TypeError):
            v = d.toKaraboValue(3)

    def test_bool(self):
        d = hashmod.Bool()
        v = d.toKaraboValue(3)
        self.assertTrue(v)
        self.assertIs(v.value, True)
        self.check_general(d, v)

    def test_vector_bool(self):
        d = hashmod.VectorBool()
        v = d.toKaraboValue([True, False, True])
        self.assertTrue(v[0])
        self.assertFalse(v[1])
        self.assertEqual(v.ndim, 1)
        self.assertEqual(v.dtype, bool)
        self.check_general(d, v)

    def test_char(self):
        d = hashmod.Char()
        v = d.toKaraboValue(b"3")
        self.assertEqual(v, 51)
        self.check_general(d, v)

    def test_vector_char(self):
        d = hashmod.VectorChar()
        v = d.toKaraboValue(b"asdf")
        self.assertTrue(v == b"asdf")
        self.assertEqual(repr(v), "b'asdf'")
        self.check_general(d, v)

    def test_ints(self):
        d = hashmod.Int8()
        v = d.toKaraboValue(3)
        self.assertEqual(v, 3)
        self.check_general(d, v)

        v = d.toKaraboValue(1.9)
        self.assertEqual(v, 1)
        v = d.toKaraboValue(1.9, strict=False)
        self.assertEqual(v, 1)

        d = hashmod.Int8(unitSymbol=Unit.METER,
                         metricPrefixSymbol=MetricPrefix.MILLI,
                         minExc=3, maxInc=6000)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue(5)
        v = d.toKaraboValue(5, strict=False)
        self.assertEqual(v, QuantityValue("5 mm"))
        v = d.toKaraboValue("5 m")
        self.assertEqual(v.magnitude, 5000)
        v = d.toKaraboValue("3.71111 m")
        self.assertEqual(v.magnitude, 3711)
        v = d.toKaraboValue("5 mm")
        self.check_general(d, v)

        with self.assertRaises(ValueError):
            v = d.toKaraboValue(2)
        with self.assertRaises(ValueError):
            v = d.toKaraboValue("1000 nm")
        with self.assertRaises(ValueError):
            v = d.toKaraboValue("7 m")

    def test_vector_ints(self):
        d = hashmod.VectorInt8()
        v = d.toKaraboValue([1, 2, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2)

        d = hashmod.VectorInt8(unitSymbol=Unit.METER,
                               metricPrefixSymbol=MetricPrefix.MILLI)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2, 3, 4])
        v = d.toKaraboValue([2, 3, 4], strict=False)
        self.assertEqual(v[1], QuantityValue("3 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = hashmod.VectorInt8(unitSymbol=Unit.METER,
                               metricPrefixSymbol=MetricPrefix.MICRO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3000)

    def test_floats(self):
        d = hashmod.Float()
        v = d.toKaraboValue(3)
        self.assertEqual(v, 3)
        self.check_general(d, v)

        d = hashmod.Float(unitSymbol=Unit.METER,
                          metricPrefixSymbol=MetricPrefix.MILLI,
                          minExc=3, maxInc=6000)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue(5)
        v = d.toKaraboValue(5, strict=False)
        self.assertEqual(v, QuantityValue("5 mm"))
        v = d.toKaraboValue("3.71111 m")
        self.assertEqual(v.magnitude, 3711.11)
        v = d.toKaraboValue("5 m")
        self.assertEqual(v.magnitude, 5000)
        self.check_general(d, v)

        with self.assertRaises(ValueError):
            v = d.toKaraboValue(2)
        with self.assertRaises(ValueError):
            v = d.toKaraboValue("1000 nm")
        with self.assertRaises(ValueError):
            v = d.toKaraboValue("7 m")

    def test_float_enum(self):
        class E(Enum):
            a = 3.3
        class F(Enum):
            a = 3.3
        d = hashmod.Int8(enum=E)
        v = d.toKaraboValue(E.a)
        self.assertIs(v.enum, E.a)
        self.assertEqual(v.value, 3.3)
        self.check_general(d, v)
        with self.assertRaises(TypeError):
            v = d.toKaraboValue(3.3)

    def test_vector_floats(self):
        d = hashmod.VectorFloat()
        v = d.toKaraboValue([1, 2, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2)

        d = hashmod.VectorFloat(unitSymbol=Unit.METER,
                                metricPrefixSymbol=MetricPrefix.MILLI)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2, 3, 4])
        v = d.toKaraboValue([2, 3, 4], strict=False)
        self.assertEqual(v[1], QuantityValue("3 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = hashmod.VectorFloat(unitSymbol=Unit.METER,
                                metricPrefixSymbol=MetricPrefix.KILO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3e-6)

    def test_complex(self):
        d = hashmod.ComplexFloat()
        v = d.toKaraboValue(3+4j)
        self.assertEqual(v, 3+4j)
        self.check_general(d, v)

        d = hashmod.ComplexFloat(unitSymbol=Unit.METER,
                          metricPrefixSymbol=MetricPrefix.MILLI)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue(5+3j)
        v = d.toKaraboValue(5+3j, strict=False)
        self.assertEqual(v.real, QuantityValue("5 mm"))
        self.assertEqual(v.imag, QuantityValue("3 mm"))
        v = d.toKaraboValue("5 m")
        self.assertEqual(v.magnitude, 5000)
        self.check_general(d, v)
        v = d.toKaraboValue("3.71111 m")
        self.assertEqual(v.magnitude, 3711.11)

    def test_vector_complex(self):
        d = hashmod.VectorComplexFloat()
        v = d.toKaraboValue([1+2j, 2+3j, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2+3j)

        d = hashmod.VectorComplexFloat(unitSymbol=Unit.METER,
                                metricPrefixSymbol=MetricPrefix.MILLI)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2+3j, 3+4j, 4])
        v = d.toKaraboValue([2+3j, 3+4j, 4], strict=False)
        self.assertEqual(v[1].real, QuantityValue("3 mm"))
        self.assertEqual(v[1].imag, QuantityValue("4 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = hashmod.VectorComplexFloat(unitSymbol=Unit.METER,
                                       metricPrefixSymbol=MetricPrefix.KILO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3e-6+4e-6j)

    def test_string(self):
        d = hashmod.String()
        v = d.toKaraboValue("bla")
        self.check_general(d, v)
        self.assertEqual(v, "bla")
        self.assertEqual(repr(v), "'bla'")

    def test_string_enum(self):
        class E(Enum):
            a = "bla"
        class F(Enum):
            a = "bla"
        d = hashmod.String(enum=E)
        v = d.toKaraboValue(E.a)
        self.assertIs(v.enum, E.a)
        self.assertEqual(v.value, "bla")
        self.check_general(d, v)
        with self.assertRaises(TypeError):
            v = d.toKaraboValue("bla")

    def test_vector_string(self):
        d = hashmod.VectorString()
        v = d.toKaraboValue(["a", "b", "c"])
        self.check_general(d, v)
        self.assertEqual(v, ["a", "b", "c"])
        self.assertEqual(v[1], "b")
        self.assertEqual(len(v), 3)


if __name__ == "__main__":
    main()
