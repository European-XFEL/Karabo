from enum import Enum
from unittest import TestCase, main

import numpy

from karabo.middlelayer_api import hash as hashmod

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

    def test_complex(self):
        d = hashmod.ComplexFloat()
        self.assertEqual(d.cast(3+7j), 3+7j)
        self.assertEqual(d.cast(3), 3)
        self.assertEqual(d.cast(3+7j).dtype, "complex64")

        d = hashmod.ComplexDouble()
        self.assertEqual(d.cast(3+7j), 3+7j)
        self.assertEqual(d.cast(3), 3)
        self.assertEqual(d.cast(3+7j).dtype, "complex128")

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

        d = hashmod.VectorComplexDouble()
        v = d.cast([1, 0.5, 3+7j])
        self.assertEqual(v.ndim, 1)
        self.assertEqual(v.dtype, "complex128")
        self.assertEqual(v.shape, (3,))
        self.assertTrue((v == [1, 0.5, 3+7j]).all())

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

    def test_vector_char(self):
        d = hashmod.VectorChar()

        self.assertEqual(d.cast(b"123"), b"123")

    def test_string(self):
        class A(Enum):
            a = "bla"
            b = "blub"

        d = hashmod.String(enum=A)
        self.assertEqual(d.cast(A.a), A.a)
        d = hashmod.String()
        self.assertEqual(d.cast("bla"), "bla")

    def test_vector_string(self):
        d = hashmod.VectorString()
        self.assertEqual(d.cast(["a", "b", ""]), ["a", "b", ""])
        with self.assertRaises(TypeError):
            d.cast([1, 2, 3])
        self.assertEqual(d.cast([]), [])


if __name__ == "__main__":
    main()
