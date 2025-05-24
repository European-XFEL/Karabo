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
import numpy as np
import pytest

from ..hash import Hash, HashList, HashType
from ..str_converter import hashtype_from_string, string_from_hashtype


@pytest.fixture(autouse=True)
def numpy_print_options():
    np.set_printoptions(suppress=True, threshold=10)
    yield
    np.set_printoptions()


def test_string_from_string():
    d = "string"
    hash_d = hashtype_from_string(HashType.String, d)
    assert hash_d == "string"
    string_d = string_from_hashtype(hash_d)
    assert string_d == d


def test_integers():
    for hash_type, value, expected in [
        (HashType.Int8, "7", np.int8(7)),
        (HashType.Int16, "15", np.int16(15)),
        (HashType.Int32, "31", np.int32(31)),
        (HashType.Int64, "63", np.int64(63)),
        (HashType.UInt8, "8", np.uint8(8)),
        (HashType.UInt16, "16", np.uint16(16)),
        (HashType.UInt32, "32", np.uint32(32)),
        (HashType.UInt64, "64", np.uint64(64)),
    ]:
        hash_d = hashtype_from_string(hash_type, value)
        assert hash_d == expected
        string_d = string_from_hashtype(hash_d)
        assert string_d == value


def test_floats():
    test_cases = [
        (HashType.Float, "1231231", np.float32(1231231.0),
         ["1.23123e+06", "1231231.0"]),
        (HashType.Float, "76233.233", np.float32(76233.233),
         ["76233.234", "76233.2", "76233.233"]),
        (HashType.Double, "1231231", np.float64(1231231.0),
         ["1.23123e+06", "1231231.0"]),
        (HashType.Double, "76233.233", np.float64(76233.233),
         ["76233.234", "76233.2", "76233.233"]),
    ]

    for hash_type, value, expected, accepted in test_cases:
        hash_d = hashtype_from_string(hash_type, value)
        assert hash_d == expected
        string_d = string_from_hashtype(hash_d)
        assert string_d in accepted


def test_none():
    d = "None"
    hash_d = hashtype_from_string(HashType.None_, d)
    assert hash_d is None
    string_d = string_from_hashtype(hash_d)
    assert string_d == "None"


def test_hash():
    d = '{"marty": "mcfly", "thunder": "lightning", "cars": 3}'
    hash_d = hashtype_from_string(HashType.Hash, d)
    assert hash_d == Hash("marty", "mcfly", "thunder", "lightning", "cars", 3)
    string_d = string_from_hashtype(hash_d)
    assert string_d == d


def test_hashlist():
    d = '[{"marty": "mcfly", "thunder": "lightning"}, {"doc": "brown"}]'
    hash_d = hashtype_from_string(HashType.VectorHash, d)
    assert isinstance(hash_d, HashList)
    assert hash_d == HashList(
        [Hash("marty", "mcfly", "thunder", "lightning"), Hash("doc", "brown")])
    string_d = string_from_hashtype(hash_d)
    assert string_d == d

    for d in ["", "[]"]:
        hash_d = hashtype_from_string(HashType.VectorHash, d)
        assert isinstance(hash_d, HashList)
        assert len(hash_d) == 0
        string_d = string_from_hashtype(hash_d)
        assert string_d == "[]"

    d = '[{"marty": "mcfly", "thunder": "lightning"}]'
    hash_d = hashtype_from_string(HashType.VectorHash, d)
    assert isinstance(hash_d, HashList)
    assert hash_d == HashList([Hash("marty", "mcfly", "thunder", "lightning")])
    string_d = string_from_hashtype(hash_d)
    assert string_d == d
