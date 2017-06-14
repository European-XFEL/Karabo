from enum import Enum
from unittest import TestCase, main

import numpy
from pint import DimensionalityError

from karabo.middlelayer import (
    AccessMode, Assignment, AccessLevel, Attribute, Bool, Char, ComplexFloat,
    decodeBinary, Double, encodeBinary, Float, Hash, Int8, Int16, MetricPrefix,
    NumpyVector, NDArray, QuantityValue, Schema, State, String, Timestamp,
    Type, Unit, unit, UInt64, VectorBool, VectorChar, VectorComplexFloat,
    VectorFloat, VectorHash, VectorInt8, VectorString)


class Tests(TestCase):
    def setUp(self):
        self.timestamp = Timestamp()

    def check_serialization(self, desc):
        """check that a descriptor can be properly serialized"""
        schema, attrs = desc.toSchemaAndAttrs(None, None)
        h = Hash("d", schema)
        h["d", ...] = attrs
        h = decodeBinary(encodeBinary(h))
        cls = Type.fromname[h["d", "valueType"]]
        newdesc = cls(strict=False, **h["d", ...])
        for k, v in desc.__dict__.items():
            if isinstance(v, Attribute):
                self.assertEqual(getattr(desc, k), getattr(newdesc, k))


    def check_general(self, desc, value):
        """check things common to all values"""
        self.assertEqual(value.descriptor, desc)
        value.timestamp = self.timestamp
        c = desc.toKaraboValue(value)
        self.assertEqual(c.timestamp, self.timestamp)
        h = Hash()
        val, attrs = desc.toDataAndAttrs(c)
        h["a"] = val
        h["a", ...].update(attrs)
        if isinstance(desc, NumpyVector):
            self.assertTrue((c == value).all())
            self.assertTrue((h["a"] == value.value).all())
        else:
            self.assertTrue(c == value)
        with self.assertRaises(TypeError):
            type(desc)(some_unknown_attr=3)
        type(desc)(strict=False, some_unknown_attr=3)
        self.check_serialization(desc)

    def test_int_enum(self):
        class E(Enum):
            a = 3
        d = Int8(enum=E)
        v = d.toKaraboValue(E.a)
        self.assertIs(v.enum, E.a)
        self.assertEqual(v.value, 3)
        self.check_general(d, v)
        with self.assertRaises(TypeError):
            v = d.toKaraboValue(3)

        class F(Enum):
            a = 3
        with self.assertRaises(TypeError):
            v = d.toKaraboValue(F.a)

    def test_bool(self):
        d = Bool()
        v = d.toKaraboValue(3)
        self.assertTrue(v)
        self.assertIs(v.value, True)
        self.check_general(d, v)

    def test_vector_bool(self):
        d = VectorBool()
        v = d.toKaraboValue([True, False, True])
        self.assertTrue(v[0])
        self.assertFalse(v[1])
        self.assertEqual(v.ndim, 1)
        self.assertEqual(v.dtype, bool)
        self.check_general(d, v)

    def test_char(self):
        d = Char()
        v = d.toKaraboValue(b"3")
        self.assertEqual(v, 51)
        self.check_general(d, v)
        with self.assertRaises(TypeError):
            d.toKaraboValue(b"123")
        with self.assertRaises(TypeError):
            d.toKaraboValue("a")

    def test_vector_char(self):
        d = VectorChar()
        v = d.toKaraboValue(b"asdf")
        self.assertTrue(v == b"asdf")
        self.assertEqual(repr(v), "b'asdf'")
        self.check_general(d, v)

    def test_ints(self):
        d = Int8()
        v = d.toKaraboValue(3)
        self.assertEqual(v, 3)
        self.check_general(d, v)

        v = d.toKaraboValue(1.9)
        self.assertEqual(v, 1)
        v = d.toKaraboValue(1.9, strict=False)
        self.assertEqual(v, 1)

        d = Int8(unitSymbol=Unit.METER, metricPrefixSymbol=MetricPrefix.MILLI,
                 minExc=3, maxInc=6000)
        v = d.toKaraboValue(5)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue(5 * unit.m / unit.m)
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

        with self.assertRaises(TypeError):
            Int16(unitSymbol="m")
        Int16(strict=False, unitSymbol="m", metricPrefixSymbol="m")

    def test_vector_ints(self):
        d = VectorInt8()
        v = d.toKaraboValue([1, 2, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2)

        d = VectorInt8(unitSymbol=Unit.METER,
                       metricPrefixSymbol=MetricPrefix.MILLI)
        v = d.toKaraboValue([2, 3, 4])
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2, 3, 4] * unit.m / unit.m)
        v = d.toKaraboValue([2, 3, 4], strict=False)
        self.assertEqual(v[1], QuantityValue("3 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = VectorInt8(unitSymbol=Unit.METER,
                       metricPrefixSymbol=MetricPrefix.MICRO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3000)

    def test_floats(self):
        d = Float()
        v = d.toKaraboValue(3)
        self.assertEqual(v, 3)
        self.check_general(d, v)

        d = Float(unitSymbol=Unit.METER, metricPrefixSymbol=MetricPrefix.MILLI,
                  minExc=3, maxInc=6000)
        v = d.toKaraboValue(5)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue(5 * unit.m / unit.m)
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

    def test_vector_floats(self):
        d = VectorFloat()
        v = d.toKaraboValue([1, 2, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2)

        d = VectorFloat(unitSymbol=Unit.METER,
                        metricPrefixSymbol=MetricPrefix.MILLI,
                        defaultValue=[1, 2, 3])
        v = d.toKaraboValue([2, 3, 4])
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2, 3, 4] * unit.m / unit.m)
        v = d.toKaraboValue([2, 3, 4], strict=False)
        self.assertEqual(v[1], QuantityValue("3 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = VectorFloat(unitSymbol=Unit.METER,
                        metricPrefixSymbol=MetricPrefix.KILO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3e-6)

    def test_complex(self):
        d = ComplexFloat()
        v = d.toKaraboValue(3+4j)
        self.assertEqual(v, 3+4j)
        self.check_general(d, v)

        d = ComplexFloat(unitSymbol=Unit.METER,
                         metricPrefixSymbol=MetricPrefix.MILLI)

        v = d.toKaraboValue(5+3j)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue((5+3j) * unit.m / unit.m)
        v = d.toKaraboValue(5+3j, strict=False)
        self.assertEqual(v.real, QuantityValue("5 mm"))
        self.assertEqual(v.imag, QuantityValue("3 mm"))
        v = d.toKaraboValue("5 m")
        self.assertEqual(v.magnitude, 5000)
        self.check_general(d, v)
        v = d.toKaraboValue("3.71111 m")
        self.assertEqual(v.magnitude, 3711.11)

    def test_vector_complex(self):
        d = VectorComplexFloat()
        v = d.toKaraboValue([1+2j, 2+3j, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2+3j)

        d = VectorComplexFloat(unitSymbol=Unit.METER,
                               metricPrefixSymbol=MetricPrefix.MILLI)
        v = d.toKaraboValue([2+3j, 3+4j, 4])
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2+3j, 3+4j, 4] * unit.m / unit.m)
        v = d.toKaraboValue([2+3j, 3+4j, 4], strict=False)
        self.assertEqual(v[1].real, QuantityValue("3 mm"))
        self.assertEqual(v[1].imag, QuantityValue("4 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = VectorComplexFloat(unitSymbol=Unit.METER,
                               metricPrefixSymbol=MetricPrefix.KILO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3e-6+4e-6j)

    def test_ndarray(self):
        d = NDArray(dtype=ComplexFloat, shape=(2, 3))
        v = d.toKaraboValue([[1, 2], [3, 4]])
        self.assertEqual(v[0, 1], 2)
        self.assertEqual(v.dtype, numpy.dtype("c8"))
        h, attrs = d.toDataAndAttrs(v)
        h = decodeBinary(encodeBinary(h))
        self.assertEqual(h["type"], 24)
        self.assertFalse(h["isBigEndian"])
        self.assertEqual(h["shape"][1], 2)
        self.assertEqual(len(h["data"]), 32)

        d = NDArray(dtype=">i2", shape=(0, 3))
        v = d.toKaraboValue(v.real)
        self.assertEqual(v.dtype, numpy.dtype(">i2"))
        self.assertEqual(v[1, 0], 3)
        schema, attrs = d.toSchemaAndAttrs(None, None)
        h = Hash("d", schema)
        h["d", ...] = attrs
        h = decodeBinary(encodeBinary(h))
        self.assertEqual(h["d", "classId"], "NDArray")
        self.assertEqual(h["d", "displayType"], "NDArray")
        self.assertEqual(h["d", "shape"][0], 0)
        self.assertEqual(h["d", "type"], 8)
        h, attrs = d.toDataAndAttrs(v)
        h = decodeBinary(encodeBinary(h))
        self.assertEqual(h["type"], 8)
        self.assertTrue(h["isBigEndian"])
        self.assertEqual(h["shape"][1], 2)
        self.assertEqual(len(h["data"]), 8)

        conv = d.toKaraboValue(h)
        self.assertEqual(conv[1, 1], 4)

        # the following tests that toKaraboValue does not copy, and
        # that the data is indeed big endian.
        h["data"][0] = 1
        conv = d.toKaraboValue(h)
        self.assertEqual(conv[0, 0], 257)

    def test_string(self):
        d = String()
        v = d.toKaraboValue("bla")
        self.check_general(d, v)
        self.assertEqual(v, "bla")
        self.assertEqual(repr(v), "'bla'")

    def test_string_enum(self):
        class E(Enum):
            a = "bla"
        d = String(enum=E)
        v = d.toKaraboValue(E.a)
        self.assertIs(v.enum, E.a)
        self.assertEqual(v.value, "bla")
        self.check_general(d, v)
        with self.assertRaises(TypeError):
            v = d.toKaraboValue("bla")

        class F(Enum):
            a = "bla"
        with self.assertRaises(TypeError):
            v = d.toKaraboValue(F.a)

    def test_vector_string(self):
        d = VectorString()
        v = d.toKaraboValue(["a", "b", "c"])
        self.check_general(d, v)
        self.assertEqual(v, ["a", "b", "c"])
        self.assertEqual(v[1], "b")
        self.assertEqual(len(v), 3)

    def test_vector_hash(self):
        rowSchema = Hash("int", None, "string", None, "vector", None,
                         "bytes", None, "array", None)
        rowSchema["int", "valueType"] = "INT32"
        rowSchema["string", "valueType"] = "STRING"
        rowSchema["vector", "valueType"] = "VECTOR_DOUBLE"
        rowSchema["bytes", "valueType"] = "VECTOR_CHAR"
        rowSchema["array", "valueType"] = "BYTE_ARRAY"

        d = VectorHash(rowSchema=Schema("rs", hash=rowSchema))
        v = d.toKaraboValue([(3, "hallo", numpy.arange(5, dtype=float),
                              b"a", b"b"),
                             (2.5, "bla", numpy.array([], dtype=float),
                              b"", b"")])
        self.assertEqual(len(v), 2)
        self.assertEqual(v[1]["int"], 2)
        self.assertEqual(v[1]["string"], "bla")
        self.assertEqual(len(v[0]["vector"]), 5)
        self.assertEqual(v[0]["vector"].dtype, float)
        self.assertEqual(v[0]["vector"][2], 2)
        self.assertEqual(len(v[1]["vector"]), 0)
        self.assertEqual(v[1]["vector"].dtype, float)
        self.assertEqual(v[1]["array"], b"")
        self.assertEqual(v[0]["bytes"], b"a")
        self.assertEqual(v.dtype.names,
                         ("int", "string", "vector", "bytes", "array"))
        d.toKaraboValue(v)
        data, _ = d.toDataAndAttrs(v)
        self.assertEqual(len(data), 2)

        h = Hash()
        h["a"] = data
        self.assertEqual(h["a"][0]["int"], 3)
        self.assertEqual(h["a"][1]["int"], 2)
        self.assertEqual(h["a"][1]["string"], "bla")
        self.assertEqual(h["a"][0]["vector"].dtype, float)

        v = d.toKaraboValue(h["a"], strict=False)
        self.assertEqual(len(v), 2)
        self.assertEqual(v[1]["int"], 2)
        self.assertEqual(v[1]["string"], "bla")
        self.assertEqual(len(v[0]["vector"]), 5)
        self.assertEqual(v[0]["vector"].dtype, float)
        self.assertEqual(v[0]["vector"][2], 2)
        self.assertEqual(len(v[1]["vector"]), 0)
        self.assertEqual(v[1]["vector"].dtype, float)
        self.assertEqual(v[1]["array"], b"")
        self.assertEqual(v[0]["bytes"], b"a")
        self.assertEqual(v.dtype.names,
                         ("int", "string", "vector", "bytes", "array"))
        d.toKaraboValue(v)

    def test_general(self):
        d = UInt64(accessMode=AccessMode.READONLY)
        self.assertIs(d.accessMode, AccessMode.READONLY)
        with self.assertRaises(TypeError):
            d = UInt64(accessMode=4)
        d = UInt64(strict=False, accessMode=4)
        self.assertIs(d.accessMode, AccessMode.RECONFIGURABLE)

        d = VectorString(assignment=Assignment.MANDATORY)
        self.assertIs(d.assignment, Assignment.MANDATORY)
        with self.assertRaises(TypeError):
            d = UInt64(assignment=0)
        d = UInt64(strict=False, assignment=0)
        self.assertIs(d.assignment, Assignment.OPTIONAL)

    def test_attributes_default(self):
        d = Double()
        self.assertIsNone(d.minExc)
        self.assertIsNone(d.maxExc)
        self.assertIsNone(d.minInc)
        self.assertIsNone(d.maxInc)
        self.assertIsNone(d.absoluteError)
        self.assertIsNone(d.relativeError)
        self.assertIsNone(d.displayedName)
        self.assertIsNone(d.alias)
        self.assertIsNone(d.description)
        self.assertIsNone(d.allowedStates)
        self.assertIsNone(d.defaultValue)
        self.assertIs(d.accessMode, AccessMode.RECONFIGURABLE)
        self.assertIs(d.assignment, Assignment.OPTIONAL)
        self.assertIs(d.requiredAccessLevel, AccessLevel.OBSERVER)
        self.assertIsNone(d.displayType)
        self.assertIs(d.unitSymbol, Unit.NUMBER)
        self.assertIs(d.metricPrefixSymbol, MetricPrefix.NONE)
        self.assertIsNone(d.options)
        self.check_serialization(d)

    def test_attributes_nodefault(self):
        d = Double(
            minExc=22, maxExc=33, minInc=11, maxInc=23,
            absoluteError=0.2, relativeError=0.3,
            displayedName="hallo", alias="something",
            description="whatever", allowedStates={State.KNOWN, State.INIT},
            defaultValue=22.5, accessMode=AccessMode.READONLY,
            assignment=Assignment.MANDATORY,
            requiredAccessLevel=AccessLevel.EXPERT, displayType="nothing",
            unitSymbol=Unit.METER, metricPrefixSymbol=MetricPrefix.MILLI,
            options=[22.3, 22.7, 22.8])

        self.assertEqual(d.minExc, 22)
        self.assertEqual(d.maxExc, 33)
        self.assertEqual(d.minInc, 11)
        self.assertEqual(d.maxInc, 23)
        self.assertEqual(d.absoluteError, 0.2)
        self.assertEqual(d.relativeError, 0.3)
        self.assertEqual(d.displayedName, "hallo")
        self.assertEqual(d.alias, "something")
        self.assertEqual(d.description, "whatever")
        self.assertEqual(d.allowedStates, {State.INIT, State.KNOWN})
        self.assertEqual(d.defaultValue, 22.5)
        self.assertIs(d.accessMode, AccessMode.READONLY)
        self.assertIs(d.assignment, Assignment.MANDATORY)
        self.assertIs(d.requiredAccessLevel, AccessLevel.EXPERT)
        self.assertEqual(d.displayType, "nothing")
        self.assertIs(d.unitSymbol, Unit.METER)
        self.assertIs(d.metricPrefixSymbol, MetricPrefix.MILLI)
        self.assertEqual(d.options, [22.3, 22.7, 22.8])
        self.check_serialization(d)

    def test_attributes_nonstrict(self):
        d = Double(
            strict=False, whatever=4, requiredAccessLevel=3,
            unitSymbol="m", metricPrefixSymbol="m")

        self.assertFalse(hasattr(d, "whatever"))
        self.assertIs(d.requiredAccessLevel, AccessLevel.EXPERT)
        self.assertIs(d.unitSymbol, Unit.METER)
        self.assertIs(d.metricPrefixSymbol, MetricPrefix.MILLI)

    def test_attributes_strict_fails(self):
        with self.assertRaises(TypeError):
            Double(requiredAccessLevel=3)
        with self.assertRaises(TypeError):
            Double(unitSymbol="m")
        with self.assertRaises(TypeError):
            Double(whatever=7)

    def test_attributes_nonstrict_fails(self):
        with self.assertRaises(ValueError):
            Double(strict=False, requiredAccessLevel=27)
        with self.assertRaises(ValueError):
            Double(strict=False, metricPrefixSymbol="asdf")


if __name__ == "__main__":
    main()
