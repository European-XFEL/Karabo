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
from unittest import TestCase, main

import numpy as np

from ..hash import Hash, HashList, HashType
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

    def test_hash(self):
        d = '{"marty": "mcfly", "thunder": "lightning", "cars": 3}'
        hash_d = hashtype_from_string(HashType.Hash, d)
        self.assertEqual(hash_d, Hash("marty", "mcfly",
                                      "thunder", "lightning",
                                      "cars", 3))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

    def test_hashlist(self):
        d = '[{"marty": "mcfly", "thunder": "lightning"}, {"doc": "brown"}]'
        hash_d = hashtype_from_string(HashType.VectorHash, d)
        assert isinstance(hash_d, HashList)
        self.assertEqual(hash_d, HashList([Hash("marty", "mcfly",
                                                "thunder", "lightning"),
                                           Hash("doc", "brown")]))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)

        # Edge case empty list
        for d in ["", "[]"]:
            hash_d = hashtype_from_string(HashType.VectorHash, d)
            assert isinstance(hash_d, HashList)
            self.assertEqual(len(hash_d), 0)
            string_d = string_from_hashtype(hash_d)
            self.assertEqual(string_d, "[]")

        # Single element in HashList
        d = '[{"marty": "mcfly", "thunder": "lightning"}]'
        hash_d = hashtype_from_string(HashType.VectorHash, d)
        assert isinstance(hash_d, HashList)
        self.assertEqual(hash_d, HashList([Hash("marty", "mcfly",
                                                "thunder", "lightning")]))
        string_d = string_from_hashtype(hash_d)
        self.assertEqual(string_d, d)


if __name__ == "__main__":
    main()
