from enum import Enum
import numpy
from pint import DimensionalityError
from unittest import TestCase, main

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, Attribute, Bool, Char, ComplexFloat,
    Configurable, Double, decodeBinary, encodeBinary, Float, Hash, Image,
    ImageData, Int16, Int8, KaraboError, LeafType, MetricPrefix, NDArray,
    NumpyVector, QuantityValue, RegexString, Schema, String, Timestamp, Type,
    UInt8, UInt64, Unit, unit, VectorBool, VectorChar, VectorComplexFloat,
    VectorFloat, VectorHash, VectorInt32, VectorInt8, VectorString)


class ArrayTestDevice(Configurable):
    array = NDArray(dtype=UInt8, shape=[20, 20, 3])


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
        h = decodeBinary(encodeBinary(h))
        if isinstance(desc, NumpyVector):
            self.assertTrue((c == value).all())
            self.assertTrue((h["a"] == value.value).all())
        else:
            self.assertTrue(c == value)
            self.assertEqual(h["a"], value.value)
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
        schema, attrs = d.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["options"], [3])

        with self.assertRaises(TypeError):
            d.toKaraboValue(3)

        class F(Enum):
            a = 3

        with self.assertRaises(TypeError):
            d.toKaraboValue(F.a)

    def test_enum_options(self):
        class E(Enum):
            a = 1
            b = 2
            c = 3

        class F(Enum):
            a = 1

        d = Int8(enum=E, options=[E.b, E.c])
        d.toKaraboValue(E.b)
        schema, attrs = d.toSchemaAndAttrs(None, None)
        self.assertEqual(list(attrs["options"]), [2, 3])

        with self.assertRaises(ValueError):
            d.toKaraboValue(E.a)
        with self.assertRaises(TypeError):
            Int8(enum=E, options=[1, 2])
        with self.assertRaises(TypeError):
            Int8(enum=E, options=[F.a])

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
        v = d.toKaraboValue("3")
        self.assertEqual(v, 51)
        with self.assertRaises(TypeError):
            d.toKaraboValue("123")

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

        d = Int16(unitSymbol=Unit.METER,
                  metricPrefixSymbol=MetricPrefix.MILLI,
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

        d = UInt64()
        v = d.toKaraboValue(10 ** 19)
        self.check_general(d, v)

    def test_options(self):
        d = Int16(options=[3, 2, 4],
                  unitSymbol=Unit.METER, metricPrefixSymbol=MetricPrefix.MILLI)
        d.toKaraboValue(3)
        with self.assertRaises(ValueError):
            d.toKaraboValue(7)
        d.toKaraboValue(4000 * unit.micrometer)
        with self.assertRaises(ValueError):
            d.toKaraboValue(4500 * unit.micrometer)

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
        v = d.toKaraboValue("3.71125 m")
        self.assertEqual(v.magnitude, 3711.25)
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
        v = d.toKaraboValue(3 + 4j)
        self.assertEqual(v, 3 + 4j)
        self.check_general(d, v)

        d = ComplexFloat(unitSymbol=Unit.METER,
                         metricPrefixSymbol=MetricPrefix.MILLI)

        v = d.toKaraboValue(5 + 3j)
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue((5 + 3j) * unit.m / unit.m)
        v = d.toKaraboValue(5 + 3j, strict=False)
        self.assertEqual(v.real, QuantityValue("5 mm"))
        self.assertEqual(v.imag, QuantityValue("3 mm"))
        v = d.toKaraboValue("5 m")
        self.assertEqual(v.magnitude, 5000)
        self.check_general(d, v)
        v = d.toKaraboValue("3.71125 m")
        self.assertEqual(v.magnitude, 3711.25)

    def test_vector_complex(self):
        d = VectorComplexFloat()
        v = d.toKaraboValue([1 + 2j, 2 + 3j, 3])
        self.check_general(d, v)
        self.assertEqual(v[1], 2 + 3j)

        d = VectorComplexFloat(unitSymbol=Unit.METER,
                               metricPrefixSymbol=MetricPrefix.MILLI)
        v = d.toKaraboValue([2 + 3j, 3 + 4j, 4])
        with self.assertRaises(DimensionalityError):
            v = d.toKaraboValue([2 + 3j, 3 + 4j, 4] * unit.m / unit.m)
        v = d.toKaraboValue([2 + 3j, 3 + 4j, 4], strict=False)
        self.assertEqual(v[1].real, QuantityValue("3 mm"))
        self.assertEqual(v[1].imag, QuantityValue("4 mm"))
        self.assertNotEqual(v[2], 4)
        self.check_general(d, v)

        d = VectorComplexFloat(unitSymbol=Unit.METER,
                               metricPrefixSymbol=MetricPrefix.KILO)
        v = d.toKaraboValue(v)
        self.assertAlmostEqual(v[1].magnitude, 3e-6 + 4e-6j)

    def test_size_vector(self):
        d = VectorString(minSize=2)
        # violate still non-string setting
        with self.assertRaises(TypeError):
            v = d.toKaraboValue([1, 1])
        with self.assertRaises(ValueError):
            v = d.toKaraboValue(['miau'])
        v = d.toKaraboValue(['wuff', 'scratch'])
        self.assertEqual(v, ['wuff', 'scratch'])

        d = VectorComplexFloat(minSize=2)
        v = d.toKaraboValue([2 + 3j, 3 + 4j, 4])
        self.assertAlmostEqual(v[1].magnitude, 3 + 4j)
        with self.assertRaises(TypeError):
            v = d.toKaraboValue(['miau'])
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([3 + 4j])

        d = VectorInt32(minSize=2, maxSize=3)
        v = d.toKaraboValue([2, 3, 4])
        self.assertAlmostEqual(v[1].magnitude, 3)
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([2])
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([2, 3, 4, 5])

        d = VectorFloat(minSize=2, maxSize=3)
        v = d.toKaraboValue([2, 3, 4])
        self.assertAlmostEqual(v[1].magnitude, 3)
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([2])
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([2, 3, 4, 5])

        d = VectorBool(minSize=2, maxSize=3)
        v = d.toKaraboValue([False, True, False])
        self.assertAlmostEqual(v[1].magnitude, True)
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([True])
        with self.assertRaises(ValueError):
            v = d.toKaraboValue([False, True, False, True])

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
        self.assertEqual(schema["shape", "defaultValue"][0], 0)
        self.assertEqual(schema["type", "defaultValue"], 8)
        self.assertEqual(schema["isBigEndian", "defaultValue"], True)

        h, attrs = d.toDataAndAttrs(v)
        h = decodeBinary(encodeBinary(h))
        self.assertTrue(h["isBigEndian"])
        self.assertEqual(h["shape"][1], 2)
        self.assertEqual(len(h["data"]), 8)

        # This checks NDArray's schema's "shape" attribute's datatype
        # This caused DAQ problems as c++ uses unsigned, while python signed
        a = ArrayTestDevice()
        s = a.getClassSchema()
        attr = s.hash["array"].getAttributes("shape")
        self.assertEqual(attr['defaultValue'].dtype, numpy.uint64)

        conv = d.toKaraboValue(h)
        self.assertEqual(conv[1, 1], 4)

        # the following tests that toKaraboValue does not copy, and
        # that the data is indeed big endian.
        h["data"][0] = 1
        conv = d.toKaraboValue(h)
        self.assertEqual(conv[0, 0], 257)

    def test_image(self):
        d = Image(dtype=UInt8, shape=(2, 2))
        v = d.toKaraboValue([[1, 2], [3, 4]])
        self.assertEqual(v.value[0, 1], 2)
        self.assertEqual(v.dtype, numpy.uint8)
        h, attrs = d.toDataAndAttrs(v)
        h = decodeBinary(encodeBinary(h))
        self.assertEqual(h["pixels.type"], 6)
        self.assertFalse(h["pixels.isBigEndian"])
        self.assertEqual(h["pixels.shape"][1], 2)
        self.assertEqual(len(h["pixels.data"]), 4)

        schema, attrs = d.toSchemaAndAttrs(None, None)
        arrayEqual = numpy.testing.assert_array_equal
        arrayEqual(schema["pixels.shape", "defaultValue"],
                   numpy.array([2, 2], dtype=numpy.uint64))
        arrayEqual(schema["dims", "defaultValue"],
                   numpy.array([2, 2], dtype=numpy.uint64))
        arrayEqual(schema["dimTypes", "defaultValue"],
                   numpy.array([], dtype=numpy.int32))
        self.assertEqual(schema["dimScales", "defaultValue"], "")
        self.assertEqual(schema["encoding", "defaultValue"], 0)
        self.assertEqual(schema["bitsPerPixel", "defaultValue"], 8)
        arrayEqual(schema["roiOffsets", "defaultValue"],
                   numpy.array([0, 0], dtype=numpy.uint64))
        arrayEqual(schema["binning", "defaultValue"],
                   numpy.array([1, 1], dtype=numpy.uint64))
        self.assertEqual(schema["rotation", "defaultValue"], 0)
        self.assertEqual(schema["flipX", "defaultValue"], False)
        self.assertEqual(schema["flipY", "defaultValue"], False)

        maxSize = 2
        self.assertEqual(schema["pixels.shape", "maxSize"], maxSize)
        self.assertEqual(schema["dims", "maxSize"], maxSize)
        self.assertEqual(schema["dimTypes", "maxSize"], maxSize)
        self.assertEqual(schema["roiOffsets", "maxSize"], maxSize)
        self.assertEqual(schema["binning", "maxSize"], maxSize)

        # Test with image data initialization to check the different
        # Schema defaults!
        d = Image(data=ImageData(numpy.zeros(shape=(2, 3, 4)), flipX=True,
                                 bitsPerPixel=32))

        schema, attrs = d.toSchemaAndAttrs(None, None)
        arrayEqual = numpy.testing.assert_array_equal
        arrayEqual(schema["pixels.shape", "defaultValue"],
                   numpy.array([2, 3, 4], dtype=numpy.uint64))
        arrayEqual(schema["dims", "defaultValue"],
                   numpy.array([2, 3, 4], dtype=numpy.uint64))

        arrayEqual(schema["dimTypes", "defaultValue"],
                   numpy.array([], dtype=numpy.int32))
        self.assertEqual(schema["dimScales", "defaultValue"], "")
        self.assertEqual(schema["encoding", "defaultValue"], 2)
        self.assertEqual(schema["bitsPerPixel", "defaultValue"], 32)
        arrayEqual(schema["roiOffsets", "defaultValue"],
                   numpy.array([0, 0, 0], dtype=numpy.uint64))
        arrayEqual(schema["binning", "defaultValue"],
                   numpy.array([1, 1, 1], dtype=numpy.uint64))
        self.assertEqual(schema["rotation", "defaultValue"], 0)
        self.assertEqual(schema["flipX", "defaultValue"], True)
        self.assertEqual(schema["flipY", "defaultValue"], False)

        maxSize = 3
        self.assertEqual(schema["pixels.shape", "maxSize"], maxSize)
        self.assertEqual(schema["dims", "maxSize"], maxSize)
        self.assertEqual(schema["dimTypes", "maxSize"], maxSize)
        self.assertEqual(schema["roiOffsets", "maxSize"], maxSize)
        self.assertEqual(schema["binning", "maxSize"], maxSize)

    def test_string(self):
        d = String()
        v = d.toKaraboValue("bla")
        self.check_general(d, v)
        self.assertEqual(v, "bla")
        self.assertEqual(repr(v), "'bla'")

    def test_regex(self):
        d = RegexString(regex=r"(0|1|[T]rue|[F]alse)")
        v = d.toKaraboValue("1")
        self.check_general(d, v)
        self.assertEqual(v, "1")
        with self.assertRaises(KaraboError):
            v = d.toKaraboValue("2")
        self.assertEqual(d.displayType, "RegexString")
        self.assertEqual(d.regex, "(0|1|[T]rue|[F]alse)")
        self.assertEqual(d.classId, "RegexString")

    def test_string_enum(self):
        class E(Enum):
            a = "bla"

        d = String(enum=E)
        v = d.toKaraboValue(E.a)
        self.assertIs(v.enum, E.a)
        self.assertEqual(v.value, "bla")
        self.check_general(d, v)
        schema, attrs = d.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["options"], ["bla"])

        with self.assertRaises(TypeError):
            d.toKaraboValue("bla")

        class F(Enum):
            a = "bla"

        with self.assertRaises(TypeError):
            d.toKaraboValue(F.a)

    def test_enum_str_options(self):
        class E(Enum):
            a = "A"
            b = "B"
            c = "C"

        class F(Enum):
            c = "C"

        d = String(enum=E, options=[E.b, E.c])
        d.toKaraboValue(E.b)
        schema, attrs = d.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["options"], ["B", "C"])

        with self.assertRaises(ValueError):
            d.toKaraboValue(E.a)
        with self.assertRaises(TypeError):
            String(enum=E, options=["B", "C"])
        with self.assertRaises(TypeError):
            String(enum=E, options=[F.c])

    def test_vector_string(self):
        d = VectorString()
        v = d.toKaraboValue(["a", "b", "c"])
        self.check_general(d, v)
        self.assertEqual(v, ["a", "b", "c"])
        self.assertEqual(v[1], "b")
        self.assertEqual(len(v), 3)

    def test_vector_hash_init(self):
        rowSchema = Hash("int", None, "string", None)
        rowSchema["int", "valueType"] = "INT32"
        rowSchema["int", "unitSymbol"] = "m"
        rowSchema["int", "metricPrefixSymbol"] = "m"
        rowSchema["string", "valueType"] = "STRING"

        d = VectorHash(rowSchema=Schema("rs", hash=rowSchema))
        # NOTE: Explicitly change order of keyValue
        v = d.toKaraboValue([Hash("int", 3, "string", "hallo"),
                             Hash("string", "hallo", "int", 20)])
        self.assertEqual(len(v), 2)
        self.assertEqual(v[1]["int"], 20 * unit.millimeter)
        self.assertEqual(v[1]["string"], "hallo")
        self.assertEqual(v.dtype.names, ("int", "string"))
        d.toKaraboValue(v)
        data, _ = d.toDataAndAttrs(v)
        self.assertEqual(len(data), 2)
        self.assertIsNone(v.timestamp)
        v.timestamp = self.timestamp
        d.toKaraboValue(v)
        data, timestamp = d.toDataAndAttrs(v)
        self.assertEqual(self.timestamp,
                         Timestamp.fromHashAttributes(timestamp))
        self.assertEqual(len(data), 2)
        self.assertEqual(d.displayType, "Table")

        # NOTE: Force error with missing key!
        with self.assertRaises(KeyError):
            v = d.toKaraboValue([Hash("int", 3),
                                 Hash("string", "hallo", "int", 20)])

        # Set a different displayType
        d = VectorHash(rowSchema=Schema("rs", hash=rowSchema),
                       displayType="Another")
        self.assertEqual(d.displayType, "Another")

        with self.assertRaises(KaraboError):
            d = VectorHash(displayType="Another")

    def test_vector_hash(self):
        rowSchema = Hash("int", None, "string", None, "vector", None,
                         "bytes", None, "array", None)
        rowSchema["int", "valueType"] = "INT32"
        rowSchema["int", "unitSymbol"] = "m"
        rowSchema["int", "metricPrefixSymbol"] = "m"
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
        self.assertEqual(v[1]["int"], 2 * unit.millimeter)
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
        self.assertIsNone(v.timestamp)
        v.timestamp = self.timestamp
        d.toKaraboValue(v)
        data, timestamp = d.toDataAndAttrs(v)
        self.assertEqual(self.timestamp,
                         Timestamp.fromHashAttributes(timestamp))
        self.assertEqual(len(data), 2)

        h = Hash()
        h["a"] = data
        self.assertEqual(h["a"][0]["int"], 3)
        self.assertEqual(h["a"][1]["int"], 2)
        self.assertEqual(h["a"][1]["string"], "bla")
        self.assertEqual(h["a"][0]["vector"].dtype, float)

        v = d.toKaraboValue(h["a"], strict=False)
        self.assertEqual(len(v), 2)
        self.assertEqual(v[1]["int"], 2 * unit.millimeter)
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
        # A reconfigurable parameter has at least `USER` access level
        self.assertIs(d.requiredAccessLevel, AccessLevel.USER)
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

    def test_leafType(self):

        state = String(
            displayType="State",
            enum=State)

        alarm = String(
            displayType="Alarm Condition",
            enum=AlarmCondition)

        # Check for state
        schema, attrs = state.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["leafType"], LeafType.State)
        # Check for Alarms
        schema, attrs = alarm.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["leafType"], LeafType.AlarmCondition)


if __name__ == "__main__":
    main()
