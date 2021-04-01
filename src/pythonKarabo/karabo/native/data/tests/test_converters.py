from unittest import TestCase, main

import numpy as np

from ..hash import HashType
from ..str_converter import hashtype_from_string, string_from_hashtype


class ConverTests(TestCase):

    def setUp(self):
        np.set_printoptions(suppress=True, threshold=10)

    def tearDown(self):
        np.set_printoptions()

    def test_string_from_string(self):
        d = "string"
        hash_d = hashtype_from_string(HashType.String, d)
        self.assertEqual(hash_d, "string")
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

    def test_integers(self):
        d = "7"
        hash_d = hashtype_from_string(HashType.Int8, d)
        self.assertEqual(hash_d, np.int8(7))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "15"
        hash_d = hashtype_from_string(HashType.Int16, d)
        self.assertEqual(hash_d, np.int16(15))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "31"
        hash_d = hashtype_from_string(HashType.Int32, d)
        self.assertEqual(hash_d, np.int32(31))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "63"
        hash_d = hashtype_from_string(HashType.Int64, d)
        self.assertEqual(hash_d, np.int64(63))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "8"
        hash_d = hashtype_from_string(HashType.UInt8, d)
        self.assertEqual(hash_d, np.uint8(8))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "16"
        hash_d = hashtype_from_string(HashType.UInt16, d)
        self.assertEqual(hash_d, np.uint16(16))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "32"
        hash_d = hashtype_from_string(HashType.UInt32, d)
        self.assertEqual(hash_d, np.uint16(32))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        d = "64"
        hash_d = hashtype_from_string(HashType.UInt64, d)
        self.assertEqual(hash_d, np.uint64(64))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

    def test_floats(self):
        d = "1231231"
        hash_d = hashtype_from_string(HashType.Float, d)
        self.assertEqual(hash_d, np.float32(1231231.0))
        string_d = string_from_hashtype(hash_d)
        self.assertIn(string_d, ["1.23123e+06", "1231231.0"])

        d = "76233.233"
        hash_d = hashtype_from_string(HashType.Float, d)
        self.assertEqual(hash_d, np.float32(76233.233))
        string_d = string_from_hashtype(hash_d)
        self.assertIn(string_d, ["76233.234", "76233.2",
                                 "76233.233"])

        d = "1231231"
        hash_d = hashtype_from_string(HashType.Double, d)
        self.assertEqual(hash_d, np.float64(1231231.0))
        string_d = string_from_hashtype(hash_d)
        self.assertIn(string_d, ["1.23123e+06", "1231231.0"])

        d = "76233.233"
        hash_d = hashtype_from_string(HashType.Double, d)
        self.assertEqual(hash_d, np.float64(76233.233))
        string_d = string_from_hashtype(hash_d)
        self.assertIn(string_d, ["76233.234", "76233.2",
                                 "76233.233"])

    def test_none(self):
        # XXX: Not fully clear, align with c++
        d = "None"
        hash_d = hashtype_from_string(HashType.None_, d)
        self.assertEqual(hash_d, None)
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, "None")


if __name__ == "__main__":
    main()
