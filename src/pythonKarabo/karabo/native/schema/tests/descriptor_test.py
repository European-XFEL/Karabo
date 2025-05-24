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
# Karabo. If not, see <https://www.mozilla.oStaterg/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from enum import Enum
from unittest import TestCase, main

import numpy as np
import pint
import pytest
from pint import DimensionalityError

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.api import KARABO_CLASS_ID_ALARM, KARABO_CLASS_ID_STATE
from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, Attribute, Bool,
    ByteArray, Char, Configurable, Double, Float, Hash, HashList, Image,
    ImageData, Int8, Int16, Int32, Int64, KaraboError, MetricPrefix, NDArray,
    NoneValue, NumpyVector, QuantityValue, RegexString, Schema, Slot, String,
    TableValue, Timestamp, Type, TypeHash, TypeNone, TypeSchema, UInt8, UInt16,
    UInt32, UInt64, Unit, VectorBool, VectorChar, VectorDouble, VectorFloat,
    VectorHash, VectorInt8, VectorInt16, VectorInt32, VectorInt64,
    VectorRegexString, VectorString, VectorUInt8, VectorUInt16, VectorUInt32,
    VectorUInt64, decodeBinary, encodeBinary, get_descriptor_from_data,
    unit_registry as unit)


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

    def test_descriptor_attrs_name(self):
        d = Int8(options=[1, 2], displayedName="Display")
        self.assertEqual(d.__class__.options.name, "options")
        self.assertEqual(d.__class__.displayedName.name, "displayedName")

    def test_bool(self):
        d = Bool()
        v = d.toKaraboValue(3)
        self.assertTrue(v)
        self.assertIs(v.value, True)
        self.check_general(d, v)

        d = Bool(defaultValue=False)
        self.assertFalse(d.defaultValue)
        with self.assertRaises(ValueError):
            d = Bool(defaultValue="")

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
        with self.assertRaises(ValueError):
            d.toKaraboValue(5000)

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

        d = Int16(
            defaultValue=7,
            unitSymbol=Unit.METER,
            metricPrefixSymbol=MetricPrefix.MILLI,
            minExc=3, maxInc=6000)
        self.assertEqual(d.minExc, 3)
        self.assertEqual(d.minExc.dtype, np.int16)
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

        # Cast properly
        d = VectorInt8(unitSymbol=Unit.METER,
                       metricPrefixSymbol=MetricPrefix.MICRO,
                       defaultValue=[1.0, 2.0, 3.5])

        self.assertEqual(d.defaultValue, [1, 2, 3])

        if pint.__version__ > "0.22":
            # Overflow for -1, we are a UInt8
            with pytest.raises(OverflowError):
                d = VectorUInt8(unitSymbol=Unit.METER,
                                metricPrefixSymbol=MetricPrefix.MICRO,
                                defaultValue=[-1.0, 2.0, 3.5])
        else:
            d = VectorUInt8(unitSymbol=Unit.METER,
                            metricPrefixSymbol=MetricPrefix.MICRO,
                            defaultValue=[-1.0, 2.0, 3.5])
            default = d.defaultValue
            self.assertEqual(default, [255, 2, 3])
            self.assertEqual(type(default[0]), np.uint8)

        # Check for unsinged int64 bit
        max_uint64 = 18446744073709551615
        d = VectorUInt64(unitSymbol=Unit.METER,
                         metricPrefixSymbol=MetricPrefix.MICRO,
                         defaultValue=[max_uint64, max_uint64])
        default = d.defaultValue
        self.assertEqual(default, [max_uint64, max_uint64])
        self.assertEqual(type(default[0]), np.uint64)

        # Test assignment with vectors
        d = VectorUInt64(unitSymbol=Unit.METER,
                         metricPrefixSymbol=MetricPrefix.MICRO,
                         defaultValue=np.array([1, 2], dtype=np.uint8))
        default = d.defaultValue
        self.assertEqual(default, [1, 2])
        self.assertEqual(type(default[0]), np.uint64)

        # Wrong dimension
        with self.assertRaises(KaraboError):
            VectorInt16(unitSymbol=Unit.METER,
                        metricPrefixSymbol=MetricPrefix.MICRO,
                        defaultValue=np.array([[1, 2], [3, 4]]))

        # Check faulty defaults
        with self.assertRaises(KaraboError):
            VectorInt8(defaultValue=2)

        # Check faulty defaults
        with self.assertRaises(KaraboError):
            VectorInt8(defaultValue="1,2,3")

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

        v = d.toKaraboValue(3.1112345)
        self.assertEqual(v.dtype, np.float32)
        self.assertAlmostEqual(v.magnitude, 3.1112345, delta=0.0001)

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
        default = d.defaultValue
        # Test that the values are casted to floats
        self.assertEqual(type(default[0]), np.float64)
        self.assertEqual(d.defaultValue, [1.0, 2.0, 3.0])

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

    def test_size_vector(self):
        d = VectorString(minSize=2)
        # violate still non-string setting
        with self.assertRaises(TypeError):
            v = d.toKaraboValue([1, 1])
        with self.assertRaises(ValueError):
            v = d.toKaraboValue(['miau'])
        v = d.toKaraboValue(['wuff', 'scratch'])
        self.assertEqual(v, ['wuff', 'scratch'])

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
        d = NDArray(dtype=Float, shape=(2, 3))
        v = d.toKaraboValue([[1, 2], [3, 4]])
        self.assertEqual(v[0, 1], 2)
        self.assertEqual(v.dtype, np.dtype("float32"))
        h, attrs = d.toDataAndAttrs(v)
        self.assertIn("__classId", attrs)
        self.assertEqual(attrs["__classId"], "NDArray")
        h = decodeBinary(encodeBinary(h))
        self.assertEqual(h["type"], 20)
        self.assertNotIn("__classId", h["type", ...])
        self.assertFalse(h["isBigEndian"])
        self.assertNotIn("__classId", h["isBigEndian", ...])
        self.assertEqual(h["shape"][1], 2)
        self.assertEqual(len(h["data"]), 16)

        d = NDArray(dtype=">i2", shape=(0, 3))
        v = d.toKaraboValue(v.real)
        self.assertEqual(v.dtype, np.dtype(">i2"))
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
        self.assertEqual(attr['defaultValue'].dtype, np.uint64)

        conv = d.toKaraboValue(h)
        self.assertEqual(conv[1, 1], 4)

        # the following tests that toKaraboValue does not copy, and
        # that the data is indeed big endian.
        h["data"][0] = 1
        conv = d.toKaraboValue(h)
        self.assertEqual(conv[0, 0], 257)

        # Handle bad data
        h.pop("data")
        conv = d.toKaraboValue(h)
        self.assertTrue(isinstance(conv, NoneValue))

    def test_image(self):
        d = Image(dtype=UInt8, shape=(2, 2))
        v = d.toKaraboValue([[1, 2], [3, 4]])
        self.assertEqual(v.value[0, 1], 2)
        self.assertEqual(v.dtype, np.uint8)
        h, attrs = d.toDataAndAttrs(v)
        self.assertIn("__classId", attrs)
        self.assertEqual(attrs["__classId"], "ImageData")
        h = decodeBinary(encodeBinary(h))
        self.assertIn("__classId", h["pixels", ...])
        self.assertNotIn("__classId", h["dims", ...])
        self.assertEqual(h["pixels", "__classId"], "NDArray")
        self.assertEqual(h["pixels.type"], 6)
        self.assertNotIn("__classId", h["pixels.type", ...])
        self.assertFalse(h["pixels.isBigEndian"])
        self.assertNotIn("__classId", h["pixels.isBigEndian", ...])
        self.assertEqual(h["pixels.shape"][1], 2)
        self.assertEqual(len(h["pixels.data"]), 4)

        schema, attrs = d.toSchemaAndAttrs(None, None)
        arrayEqual = np.testing.assert_array_equal
        arrayEqual(schema["pixels.shape", "defaultValue"],
                   np.array([2, 2], dtype=np.uint64))
        arrayEqual(schema["dims", "defaultValue"],
                   np.array([2, 2], dtype=np.uint64))
        self.assertEqual(schema["encoding", "defaultValue"], 0)
        self.assertEqual(schema["bitsPerPixel", "defaultValue"], 8)
        arrayEqual(schema["roiOffsets", "defaultValue"],
                   np.array([0, 0], dtype=np.uint64))
        arrayEqual(schema["binning", "defaultValue"],
                   np.array([1, 1], dtype=np.uint64))
        self.assertEqual(schema["rotation", "defaultValue"], 0)
        self.assertEqual(schema["flipX", "defaultValue"], False)
        self.assertEqual(schema["flipY", "defaultValue"], False)

        maxSize = 2
        self.assertEqual(schema["pixels.shape", "maxSize"], maxSize)
        self.assertEqual(schema["dims", "maxSize"], maxSize)
        self.assertEqual(schema["roiOffsets", "maxSize"], maxSize)
        self.assertEqual(schema["binning", "maxSize"], maxSize)

        # Test with image data initialization to check the different
        # Schema defaults!
        d = Image(data=ImageData(np.zeros(shape=(2, 3, 4)), flipX=True,
                                 bitsPerPixel=32))

        schema, attrs = d.toSchemaAndAttrs(None, None)
        arrayEqual = np.testing.assert_array_equal
        arrayEqual(schema["pixels.shape", "defaultValue"],
                   np.array([2, 3, 4], dtype=np.uint64))
        arrayEqual(schema["dims", "defaultValue"],
                   np.array([2, 3, 4], dtype=np.uint64))
        self.assertEqual(schema["encoding", "defaultValue"], 2)
        self.assertEqual(schema["bitsPerPixel", "defaultValue"], 32)
        arrayEqual(schema["roiOffsets", "defaultValue"],
                   np.array([0, 0, 0], dtype=np.uint64))
        arrayEqual(schema["binning", "defaultValue"],
                   np.array([1, 1, 1], dtype=np.uint64))
        self.assertEqual(schema["rotation", "defaultValue"], 0)
        self.assertEqual(schema["flipX", "defaultValue"], True)
        self.assertEqual(schema["flipY", "defaultValue"], False)

        maxSize = 3
        self.assertEqual(schema["pixels.shape", "maxSize"], maxSize)
        self.assertEqual(schema["dims", "maxSize"], maxSize)
        self.assertEqual(schema["roiOffsets", "maxSize"], maxSize)
        self.assertEqual(schema["binning", "maxSize"], maxSize)

    def test_string(self):
        d = String()
        v = d.toKaraboValue("bla")
        self.check_general(d, v)
        self.assertEqual(v, "bla")
        self.assertEqual(repr(v), "'bla'")

        for v in ([], 1, {}, Schema(), 2.3, True, False):
            with pytest.raises(ValueError) as exc:
                d = String(defaultValue=v)
            assert "A string is required for Attribute" in str(exc)

    def test_vector_regex(self):
        d = VectorRegexString(regex=r"[a-z]+")
        v = d.toKaraboValue(["a", "b", "c"])
        self.check_general(d, v)
        self.assertEqual(v, ["a", "b", "c"])

        d = VectorRegexString(regex=r"[a-z]+:[a-z]+")
        v = d.toKaraboValue(["device:output"])
        self.check_general(d, v)
        self.assertEqual(v, ["device:output"])
        with self.assertRaises(KaraboError):
            v = d.toKaraboValue("[nodevice:valid:invalid]")
        self.assertEqual(d.regex, "[a-z]+:[a-z]+")
        self.assertEqual(d.displayType, "VectorRegexString")
        self.assertEqual(d.classId, "VectorRegexString")

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
        with self.assertRaises(KaraboError):
            d = VectorString(defaultValue="")
        with self.assertRaises(ValueError):
            d = VectorString(defaultValue=[1])
        with self.assertRaises(ValueError):
            d = VectorString(defaultValue=["a", 1])
        VectorString(defaultValue=[])
        VectorString(defaultValue=["a"])

    def test_vector_hash_init(self):
        # Test vector hash without schema
        with self.assertRaises(KaraboError):
            d = VectorHash()

        # Test with empty row schema
        with self.assertRaises(KaraboError):
            d = VectorHash(rowSchema=Schema("rs", hash=Hash()))

        rowSchema = Hash("int", None, "string", None)
        rowSchema["int", "valueType"] = "INT32"
        rowSchema["int", "unitSymbol"] = "m"
        rowSchema["int", "defaultValue"] = 27
        rowSchema["int", "metricPrefixSymbol"] = "m"
        rowSchema["int", "accessMode"] = 4
        rowSchema["string", "valueType"] = "STRING"
        rowSchema["string", "accessMode"] = 1

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
        self.assertIsInstance(data, HashList)
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

        # Set an empty list as vector hash
        v = d.toKaraboValue([])
        data, _ = d.toDataAndAttrs(v)
        self.assertIsInstance(data, HashList)

        bindings = d.bindings
        integerBinding = bindings["int"]
        stringBinding = bindings["string"]
        self.assertIsInstance(integerBinding, Int32)
        attrs = integerBinding.attributes
        self.assertIs(attrs["unitSymbol"], Unit.METER)
        self.assertIs(attrs["metricPrefixSymbol"], MetricPrefix.MILLI)

        # Another one without units
        self.assertIsInstance(stringBinding, String)
        attrs = stringBinding.attributes
        self.assertIs(attrs["metricPrefixSymbol"], MetricPrefix.NONE)

        # Check the sizes of a vector hash
        d = VectorHash(rowSchema=Schema("rs", hash=rowSchema),
                       minSize=1, maxSize=2)
        v = d.toKaraboValue([Hash("int", 3, "string", "hallo"),
                             Hash("string", "hallo", "int", 20)])
        data, _ = d.toDataAndAttrs(v)
        self.assertIsInstance(data, HashList)

        # Test the default hash
        default = d.default_row
        self.assertIsInstance(default, Hash)
        self.assertEqual(default["int"], 27)
        self.assertEqual(default["string"], "")

        with self.assertRaises(ValueError):
            v = d.toKaraboValue([])

        with self.assertRaises(ValueError):
            v = d.toKaraboValue([Hash("int", 3, "string", "hallo"),
                                 Hash("string", "hallo", "int", 20),
                                 Hash("string", "now", "int", 30)])

    def test_vector_hash(self):
        rowSchema = Hash("int", None, "string", None, "vector", None)
        rowSchema["int", "valueType"] = "INT32"
        rowSchema["int", "unitSymbol"] = "m"
        rowSchema["int", "metricPrefixSymbol"] = "m"
        rowSchema["string", "valueType"] = "STRING"
        rowSchema["vector", "valueType"] = "VECTOR_DOUBLE"
        d = VectorHash(rowSchema=Schema("rs", hash=rowSchema))
        v = d.toKaraboValue([(3, "hallo", np.arange(5, dtype=float)),
                             (2.5, "bla", np.array([], dtype=float))])
        self.assertEqual(len(v), 2)
        self.assertEqual(v[1]["int"], 2 * unit.millimeter)
        self.assertEqual(v[1]["string"], "bla")
        self.assertEqual(len(v[0]["vector"]), 5)
        self.assertEqual(v[0]["vector"].dtype, float)
        self.assertEqual(v[0]["vector"][2], 2)
        self.assertEqual(len(v[1]["vector"]), 0)
        self.assertEqual(v[1]["vector"].dtype, float)
        self.assertEqual(v.dtype.names,
                         ("int", "string", "vector"))
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
        self.assertEqual(v.dtype.names,
                         ("int", "string", "vector"))
        d.toKaraboValue(v)

        class A(Configurable):
            d = VectorHash(rowSchema=Schema("rs", hash=rowSchema))

        a = A()
        a.d = [(3, "hallo", np.arange(5, dtype=float)),
               (2.5, "bla", np.array([], dtype=float))]
        self.assertEqual(v[0]["int"], 3 * unit.millimeter)
        self.assertEqual(v[0]["string"], "hallo")

        a.d[0] = Hash("int", 23, "string", "karabo", "vector",
                      np.arange(23, dtype=float))
        v = a.d
        self.assertIsInstance(v, TableValue)
        self.assertEqual(v[0]["int"], 23 * unit.millimeter)
        self.assertEqual(v[0]["string"], "karabo")
        self.assertEqual(len(v[0]["vector"]), 23)

        # Check row index
        index = v.columnIndex("string")
        self.assertEqual(index, 1)

        h = v.default_row()
        self.assertIsInstance(h, Hash)
        n = v.default_row()
        self.assertIsNot(h, n)
        h["int"] = 27
        self.assertFalse(h.fullyEqual(n))

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
        # A reconfigurable parameter has at least `OPERATOR` access level
        self.assertIs(d.requiredAccessLevel, AccessLevel.OPERATOR)
        self.assertIsNone(d.displayType)
        self.assertIs(d.unitSymbol, Unit.NUMBER)
        self.assertIs(d.metricPrefixSymbol, MetricPrefix.NONE)
        self.assertIsNone(d.options)

        attributes = d.attributes
        self.assertIsNone(attributes.get('minExc'))
        self.assertIsNone(attributes.get('minInc'))
        self.assertIsNone(attributes.get('maxExc'))
        self.assertIsNone(attributes.get('maxInc'))
        self.assertIsNone(attributes.get('displayType'))
        self.assertIsNone(attributes.get('defaultValue'))
        self.assertIsNone(attributes.get('allowedStates'))
        self.assertIsNone(attributes.get('classId'))
        self.assertIsNone(attributes.get('description'))
        self.assertIsNone(attributes.get('alias'))
        self.assertIsNone(attributes.get('relativeError'))
        self.assertIsNone(attributes.get('absoluteError'))
        self.assertIs(attributes['accessMode'],
                      AccessMode.RECONFIGURABLE)
        self.assertIs(attributes['assignment'],
                      Assignment.OPTIONAL)
        self.assertIs(attributes['requiredAccessLevel'],
                      AccessLevel.OPERATOR)
        self.assertIs(attributes['unitSymbol'], Unit.NUMBER)
        self.assertIs(attributes['metricPrefixSymbol'], MetricPrefix.NONE)

        self.check_serialization(d)

    def test_alias_attribute(self):
        """ An alias attribute can have multiple types, string, etc..."""
        d = Double(
            displayedName="Double",
            alias="string")
        self.assertEqual(d.alias, "string")

        # But might be integers as well
        d = Double(
            displayedName="Double",
            alias=0x2)
        self.assertEqual(d.alias, 0x2)
        self.assertEqual(d.attributes['alias'], 0x2)

        d = Double(
            displayedName="Double")
        _, attrs = d.toSchemaAndAttrs(None, None)
        self.assertNotIn("alias", attrs)
        self.assertIsNone(d.attributes.get('alias'))

    def test_error_attributes(self):
        d = Double(
            absoluteError=0.2, relativeError=0.3)
        self.assertIsNotNone(d)

        # Negative abs error not allowed
        with self.assertRaises(KaraboError):
            d = Double(absoluteError=-0.2)

        # zero abs error not allowed
        with self.assertRaises(KaraboError):
            d = Double(absoluteError=0.0)

        # neg rel error not allowed
        with self.assertRaises(KaraboError):
            d = Double(relativeError=-0.1)

        # zero rel error not allowed
        with self.assertRaises(KaraboError):
            d = Double(relativeError=0.0)

        # Test string input for errors
        with self.assertRaises(TypeError):
            d = Double(relativeError="StringIsWrong")

        with self.assertRaises(TypeError):
            d = Double(absoluteError="StringIsWrong")

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

        attributes = d.attributes
        self.assertEqual(attributes['minExc'], 22)
        self.assertEqual(attributes['maxExc'], 33)
        self.assertEqual(attributes['minInc'], 11)
        self.assertEqual(attributes['maxInc'], 23)
        self.assertEqual(attributes['absoluteError'], 0.2)
        self.assertEqual(attributes['relativeError'], 0.3)
        self.assertEqual(attributes['displayedName'], "hallo")
        self.assertEqual(attributes['alias'], "something")
        self.assertEqual(attributes['description'], "whatever")
        self.assertEqual(len(attributes['allowedStates']), 2)
        self.assertIn('INIT', attributes['allowedStates'])
        self.assertIn('KNOWN', attributes['allowedStates'])
        self.assertEqual(attributes['defaultValue'], 22.5)
        self.assertIs(attributes['accessMode'], AccessMode.READONLY)
        self.assertIs(attributes['assignment'], Assignment.MANDATORY)
        self.assertIs(attributes['requiredAccessLevel'], AccessLevel.EXPERT)
        self.assertEqual(attributes['displayType'], "nothing")
        self.assertIs(attributes['unitSymbol'], Unit.METER)
        self.assertIs(attributes['metricPrefixSymbol'], MetricPrefix.MILLI)
        self.assertEqual(attributes['options'], [22.3, 22.7, 22.8])

        self.check_serialization(d)

    def test_attributes_nonstrict(self):
        d = Double(
            strict=False, whatever=4, requiredAccessLevel=2,
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

    def test_enum_leaf_classId(self):

        state = String(
            displayType="State",
            enum=State)

        alarm = String(
            displayType="Alarm Condition",
            enum=AlarmCondition)

        # Check for state
        _, attrs = state.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["classId"], KARABO_CLASS_ID_STATE)
        # Check for Alarms
        _, attrs = alarm.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["classId"], KARABO_CLASS_ID_ALARM)

    def _min_max_desc(self, d, min_, max_):
        minv, maxv = d.getMinMax()
        self.assertEqual(minv, min_)
        self.assertEqual(maxv, max_)
        with self.assertRaises(ValueError):
            d.check(minv - 1)
        with self.assertRaises(ValueError):
            d.check(maxv + 1)

    def test_min_max_descriptors(self):
        self._min_max_desc(UInt8(), 0, 255)
        self._min_max_desc(UInt16(), 0, 65535)
        self._min_max_desc(UInt32(), 0, 4294967295)
        self._min_max_desc(UInt64(), 0, 18446744073709551615)

        self._min_max_desc(Int8(), -128, 127)
        self._min_max_desc(Int16(), -32768, 32767)
        self._min_max_desc(Int32(), -2147483648, 2147483647)
        self._min_max_desc(
            Int64(), -9223372036854775808, 9223372036854775807)

        # Test that specials will raise
        for desc in [Double, Int64]:
            with self.assertRaises(ValueError):
                desc(maxInc=np.inf)
            with self.assertRaises(ValueError):
                desc(minInc=-np.inf)

        with self.assertRaises(ValueError):
            Int64(maxInc=np.nan)
        with self.assertRaises(ValueError):
            Int64(minInc=-np.nan)

        d = Double(minInc=-np.nan)
        self.assertIsNotNone(d)
        d = Double(maxInc=np.nan)
        self.assertIsNotNone(d)

    def test_slot(self):
        """Test the slot descriptor"""
        d = Slot()
        self.assertIsNotNone(d)
        h, attrs = d.toSchemaAndAttrs(None, None)
        self.assertEqual(attrs["classId"], "Slot")
        self.assertEqual(attrs["displayType"], "Slot")
        self.assertEqual(attrs["nodeType"], 1)

    def test_initialize_wrong(self):
        """Test the initialization of a descriptor with wrong input"""
        # Allowed states must be states
        with self.assertRaises(TypeError):
            d = UInt16(allowedStates=["INIT", "MOVING"])
        d = UInt16(allowedStates=[State.INIT, State.MOVING])
        self.assertIsNotNone(d)

        # Tags must be a list of strings
        with self.assertRaises(TypeError):
            d = UInt16(tags=[1, 2])
        d = UInt8(tags=["1", "2"])
        self.assertIsNotNone(d)

        d = UInt16(defaultValue=2)
        self.assertIsNotNone(d)

        # Integers can have Enums and defaultValue of Enums, although
        # their defaultValue dtype is integer
        visiblity = Int32(defaultValue=AccessLevel.EXPERT, enum=AccessLevel,
                          options=[level for level in AccessLevel])
        self.assertIsNotNone(visiblity)

        # Setting wrong Enum to enum type Attribute, descriptor validation
        with self.assertRaises(TypeError):
            d = Int16(defaultValue=2,
                      requiredAccessLevel=AccessMode.RECONFIGURABLE)

        with self.assertRaises(TypeError):
            d = Int16(defaultValue=2, requiredAccessLevel=2)

        # In non strict case we convert
        d = Int16(strict=False, defaultValue=2, requiredAccessLevel=1)
        self.assertEqual(d.requiredAccessLevel, AccessLevel.OPERATOR)

        d = Int16(strict=False, defaultValue=2,
                  archivePolicy=ArchivePolicy.EVERY_EVENT)
        self.assertEqual(d.archivePolicy, ArchivePolicy.EVERY_EVENT)

    def test_descriptor_from_data(self):
        data = [
            ("5", String),
            (np.int8(7), Int8),
            (np.int16(15), Int16),
            (np.int32(31), Int32),
            (np.int64(63), Int64),
            (np.uint8(8), UInt8),
            (np.uint16(16), UInt16),
            (np.uint32(32), UInt32),
            (np.uint64(64), UInt64),
            (np.array([1, 0, 1], dtype=np.bool_), VectorBool),
            (np.array([1, 2, 3], dtype=np.int8), VectorInt8),
            (np.array([1, 2, 3], dtype=np.int16), VectorInt16),
            (np.array([1, 2, 3], dtype=np.int32), VectorInt32),
            (np.array([1, 2, 3], dtype=np.int64), VectorInt64),
            (np.array([1, 2, 3], dtype=np.uint8), VectorUInt8),
            (np.array([1, 2, 3], dtype=np.uint16), VectorUInt16),
            (np.array([1, 2, 3], dtype=np.uint32), VectorUInt32),
            (np.array([1, 2, 3], dtype=np.uint64), VectorUInt64),
            (np.array([1.21, 0.1, 1.22], dtype=np.float32), VectorFloat),
            (np.array([1.22, 2.7, 3.55], dtype=np.float64), VectorDouble),
            (HashList(), VectorHash),
            ([Hash("position", 5), Hash("position", 5)], VectorHash),
            (Schema(name="empty"), TypeSchema),
            (Hash("position", 5.1), TypeHash),
            (np.bool_(1), Bool),
            (False, Bool),
            (1.212, Double),
            (1, Int32),
            (np.float32(7), Float),
            (np.float64(33), Double),
            (None, TypeNone),
            (np.array([1.22, 2.7, 3.55]).data, ByteArray),
            (bytes((1, 2, 3, 4)), VectorChar)
        ]

        for d, desc in data:
            self.assertEqual(desc, get_descriptor_from_data(d))

    def assert_numpy_attributes(self, descr_klass, minInc, maxInc,
                                minExc, maxExc, defaultValue,
                                dtype=None):

        descriptor = descr_klass(
            defaultValue=defaultValue,
            minInc=minInc,
            maxInc=maxInc,
            minExc=minExc,
            maxExc=maxExc)

        dtype = dtype or descriptor.numpy
        self.assertEqual(descriptor.defaultValue, defaultValue)
        self.assertEqual(descriptor.defaultValue.dtype, dtype)

        # Minimum and Maximum
        self.assertEqual(descriptor.minInc, minInc)
        self.assertEqual(descriptor.minInc.dtype, dtype)
        self.assertEqual(descriptor.maxInc, maxInc)
        self.assertEqual(descriptor.maxInc.dtype, dtype)
        self.assertEqual(descriptor.minExc, minExc)
        self.assertEqual(descriptor.minExc.dtype, dtype)
        self.assertEqual(descriptor.maxExc, maxExc)
        self.assertEqual(descriptor.maxExc.dtype, dtype)

        # Assert type
        self.assertEqual(descriptor.__class__, Type.types[descriptor.number])
        self.assertEqual(descriptor.__class__,
                         Type.strs[np.dtype(descriptor.numpy).str])
        self.assertEqual(descriptor.__class__,
                         Type.fromname[descriptor.hashname()])

    def test_descriptor_numpy_attributes(self):
        for desc in [Int8, Int16, Int32, Int64,
                     UInt8, UInt16, UInt32, UInt64]:
            self.assert_numpy_attributes(
                desc, 1, 10, 0, 11,  # Desc, minInc, maxInc, minExc, maxExc,
                3)  # value

        # We assert numpy attributes as well for Float and Double, they will
        # be casted, but all attribute for floating points have 64 bit!
        for desc in [Float, Double]:
            self.assert_numpy_attributes(
                desc, 1.30, 10.02,  # Desc, minInc, maxInc,
                0.12, 11.37,  # minExc, maxExc,
                3.07, dtype=np.float64)  # default value, dtype

        # Attributes are casted to their numpy type
        d = Int8(defaultValue=8.2)
        self.assertEqual(d.defaultValue, 8)
        d = Double(defaultValue=8)
        self.assertEqual(d.defaultValue, 8.0)

        # But wrong value not in limits provides an exception
        with self.assertRaises(ValueError):
            d = UInt8(defaultValue=-1)
        with self.assertRaises(ValueError):
            d = Int8(defaultValue=-301)
        with self.assertRaises(ValueError):
            d = Int8(defaultValue=1301)

        # Floating points can have absolute and relative error
        d = Float(
            absoluteError=0.123,
            relativeError=0.44)
        self.assertEqual(d.absoluteError, 0.123)
        self.assertEqual(d.relativeError, 0.44)
        self.assertEqual(d.absoluteError.dtype, np.float64)
        self.assertEqual(d.relativeError.dtype, np.float64)

        d = Double(
            absoluteError=0.01,
            relativeError=0.12)
        self.assertEqual(d.absoluteError, 0.01)
        self.assertEqual(d.relativeError, 0.12)
        self.assertEqual(d.absoluteError.dtype, np.float64)
        self.assertEqual(d.relativeError.dtype, np.float64)


if __name__ == "__main__":
    main()
