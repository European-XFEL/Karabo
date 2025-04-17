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
from enum import Enum
from unittest import TestCase, main

from karabo.native import Configurable, Hash, HashList, Schema
from karabo.native.schema import descriptors as hashmod


class RowSchema(Configurable):
    akey = hashmod.String(defaultValue="")


class Tests(TestCase):
    def test_int_enum(self):
        class A(Enum):
            a = 3
            b = 5

        d = hashmod.Int32(enum=A)
        self.assertIs(d.cast(A.b), A.b)
        with self.assertRaises(TypeError):
            d.cast(3)

    def test_int(self):
        d = hashmod.UInt32()
        self.assertEqual(d.cast(3), 3)
        self.assertEqual(d.cast(3).dtype, "uint32")

        d = hashmod.Int16(maxInc=22)
        self.assertEqual(d.cast(3), 3)
        self.assertEqual(d.cast(12.9), 12)
        self.assertEqual(d.cast(3).dtype, "int16")
        self.assertEqual(d.cast(22), 22)
        with self.assertRaises(ValueError):
            d.cast(55)

        d = hashmod.Int16()
        v = d.cast(22)
        d = hashmod.UInt32()
        self.assertEqual(d.cast(v), 22)
        self.assertEqual(d.cast(v).dtype, "uint32")

        d = hashmod.UInt8(minInc=33)
        self.assertEqual(d.cast(33).dtype, "uint8")
        with self.assertRaises(ValueError):
            d.cast(32.9)

        d = hashmod.Int8(minExc=7)
        self.assertEqual(d.cast(8).dtype, "int8")
        with self.assertRaises(ValueError):
            d.cast(7)

        d = hashmod.Int64(maxExc=-55)
        self.assertEqual(d.cast(-100).dtype, "int64")
        with self.assertRaises(ValueError):
            d.cast(-55)

    def test_float(self):
        d = hashmod.Float()
        self.assertEqual(d.cast(3), 3)
        self.assertEqual(d.cast(3).dtype, "float32")

        d = hashmod.Int16()
        v = d.cast(22)
        d = hashmod.Float()
        self.assertEqual(d.cast(v), 22)
        self.assertEqual(d.cast(v).dtype, "float32")

        d = hashmod.Float(maxInc=22)
        self.assertEqual(d.cast(3), 3)
        self.assertAlmostEqual(d.cast(12.9), 12.9, 6)
        self.assertEqual(d.cast(22), 22)
        with self.assertRaises(ValueError):
            d.cast(22.1)

        d = hashmod.Double(minInc=33)
        self.assertEqual(d.cast(33).dtype, "float64")
        self.assertEqual(d.cast(52.9), 52.9)
        with self.assertRaises(ValueError):
            d.cast(32.9)

        d = hashmod.Double(minExc=7)
        self.assertEqual(d.cast(7.001).dtype, "float64")
        with self.assertRaises(ValueError):
            d.cast(7)

        d = hashmod.Double(maxExc=-55)
        self.assertEqual(d.cast(-100).dtype, "float64")
        with self.assertRaises(ValueError):
            d.cast(-55)

    def test_vector(self):
        d = hashmod.VectorUInt8()
        v = d.cast([1, 2, 3])
        self.assertEqual(v.ndim, 1)
        self.assertEqual(v.dtype, "uint8")
        self.assertEqual(v.shape, (3,))
        self.assertTrue((v == [1, 2, 3]).all())

        d = hashmod.VectorFloat()
        v = d.cast([1, 0.5, 3])
        self.assertEqual(v.ndim, 1)
        self.assertEqual(v.dtype, "float32")
        self.assertEqual(v.shape, (3,))
        self.assertTrue((v == [1, 0.5, 3]).all())

        d = hashmod.VectorDouble()
        v = d.cast(v)
        self.assertEqual(v.ndim, 1)
        self.assertEqual(v.dtype, "float64")
        self.assertEqual(v.shape, (3,))
        self.assertTrue((v == [1, 0.5, 3]).all())

    def test_bool(self):
        d = hashmod.Bool()

        self.assertTrue(d.cast(1))
        self.assertFalse(d.cast(0))
        self.assertTrue(d.cast(True))
        self.assertFalse(d.cast(False))
        self.assertIs(d.cast(0), False)

    def test_vector_bool(self):
        d = hashmod.VectorBool()

        self.assertTrue((d.cast([True, False]) == [True, False]).all())
        self.assertTrue((d.cast([1, 0]) == [True, False]).all())
        self.assertEqual(d.cast([1, 0]).dtype, "bool")
        self.assertTrue((d.cast(["true", "false"]) == [True, False]).all())
        self.assertTrue((d.cast(["1", "0"]) == [True, False]).all())

    def test_char(self):
        d = hashmod.Char()

        self.assertEqual(d.cast(33), "!")
        self.assertEqual(d.cast(";"), ";")
        with self.assertRaises(TypeError):
            d.cast("bla")
        with self.assertRaises(TypeError):
            d.cast("")
        self.assertEqual(d.cast(b"a"), "a")
        with self.assertRaises(TypeError):
            d.cast(b"asd")
        with self.assertRaises(TypeError):
            d.cast(b"")

    def test_bytearray(self):
        d = hashmod.ByteArray()
        self.assertEqual(d.cast(bytearray("ginger", 'utf-8')),
                         bytearray(b'ginger'))
        self.assertEqual(d.cast("ginger"), bytearray(b'ginger'))
        self.assertEqual(d.cast(1), bytearray(b'\x00'))

    def test_vector_char(self):
        d = hashmod.VectorChar()
        self.assertEqual(d.cast(b"123"), b"123")
        self.assertEqual(d.cast("123"), b"123")
        self.assertEqual(d.cast(bytes("123", "utf-8")), b"123")

    def test_string(self):
        class A(Enum):
            a = "bla"
            b = "blub"

        d = hashmod.String(enum=A)
        self.assertEqual(d.cast(A.a), A.a)
        d = hashmod.String()
        self.assertEqual(d.cast("bla"), "bla")
        self.assertEqual(d.cast(1), "1")

    def test_vector_string(self):
        d = hashmod.VectorString()
        self.assertEqual(d.cast(["a", "b", ""]), ["a", "b", ""])
        with self.assertRaises(TypeError):
            d.cast([1, 2, 3])
        self.assertEqual(d.cast([]), [])

    def test_hash(self):
        d = hashmod.TypeHash()
        h = Hash("success", True)
        self.assertEqual(d.cast(h), h)
        with self.assertRaises(TypeError):
            d.cast({"sucess": True})

    def test_vector_hash(self):
        d = hashmod.VectorHash(rows=RowSchema)
        hl = [Hash("success", True)]
        self.assertEqual(d.cast(hl), hl)
        # It is equal, but the type is hashlist
        # No cast to the rowschema of a vector hash is done
        # This is validated on value setting of the descriptor for each
        # column, but not part of the generic vector hash cast.
        self.assertIsInstance(d.cast(hl), HashList)

        # Only the Hash type is validated
        with self.assertRaises(TypeError):
            hl = [{"success": True}]
            self.assertEqual(d.cast(hl), hl)

    def test_schema(self):
        d = hashmod.TypeSchema()
        s = Schema(name="success")
        self.assertEqual(d.cast(s), s)
        with self.assertRaises(TypeError):
            d.cast(s.hash)

    def test_none(self):
        d = hashmod.TypeNone()
        self.assertEqual(d.cast(None), None)
        with self.assertRaises(TypeError):
            d.cast(1)
        with self.assertRaises(TypeError):
            d.cast("")

    def test_slot(self):
        d = hashmod.Slot()
        self.assertEqual(d.cast("anything"), Hash())
        self.assertEqual(d.cast(121313), Hash())


if __name__ == "__main__":
    main()
