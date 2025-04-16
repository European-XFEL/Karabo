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
from functools import partial
from struct import Struct, pack

import numpy as np

from .hash import _get_hash_num_from_data
from .typenums import HashType

__all__ = ['encodeBinary', 'writeBinary']

packI = Struct('I').pack
packB = Struct('B').pack


def yieldKey(key):
    key = key.encode()
    yield packB(len(key))
    yield key


def encodeBinary(data):
    """return the binary serialization of hash `data`"""
    hash_type = _get_hash_num_from_data(data)
    return b"".join(__WRITER_MAP[hash_type](data))


def writeBinary(data, file):
    """write the hash in `data` into `file`"""
    hash_type = _get_hash_num_from_data(data)
    for d in __WRITER_MAP[hash_type](data):
        file.write(d)


def yield_binary_hash(data):
    yield packI(len(data))
    for k, v, attrs in data.iterall():
        yield from yieldKey(k)
        hash_type = _get_hash_num_from_data(v)
        yield pack('II', hash_type, len(attrs))
        for ak, av in attrs.items():
            atype = _get_hash_num_from_data(av)
            yield from yieldKey(ak)
            yield packI(atype)
            yield from __WRITER_MAP[atype](av)
        yield from __WRITER_MAP[hash_type](v)


def yield_binary_schema(data):
    binary = b''.join(yield_binary_hash(data.hash))
    name = data.name.encode()
    yield pack('IB', len(binary) + len(name) + 1, len(name))
    yield name
    yield binary


def yield_binary_char(data):
    assert len(data) == 1
    yield data.encode("ascii")


def yield_binary_vector_char(data):
    yield packI(len(data))
    yield data


def yield_binary_string(data):
    yield from yield_binary_vector_char(data.encode())


def yield_binary_numpy_vector(data, numpy=None):
    if not isinstance(data, np.ndarray) or data.dtype != numpy:
        data = np.array(data, dtype=numpy)
    yield packI(len(data))
    yield data.data


def binary_simple_factory(fmt):
    pack = Struct(fmt).pack

    def yield_binary_simple(data):
        yield pack(data)
    return yield_binary_simple


def yield_binary_bool(data):
    yield b'\1' if data else b'\0'


def yield_binary_vector_hash(data):
    yield packI(len(data))
    for d in data:
        yield from yield_binary_hash(d)


def yield_binary_vector_string(data):
    yield packI(len(data))
    for d in data:
        yield from yield_binary_string(d)


def yield_binary_empty(data):
    yield b'\0\0\0\0'


def yield_binary_bytearray(data):
    mv = memoryview(data)
    yield packI(mv.nbytes)
    yield mv


__WRITER_MAP = {k.value: v for k, v in {
    HashType.Bool: yield_binary_bool,
    HashType.Char: yield_binary_char,
    HashType.Int8: binary_simple_factory('b'),
    HashType.UInt8: binary_simple_factory('B'),
    HashType.Int16: binary_simple_factory('h'),
    HashType.UInt16: binary_simple_factory('H'),
    HashType.Int32: binary_simple_factory('i'),
    HashType.UInt32: binary_simple_factory('I'),
    HashType.Int64: binary_simple_factory('q'),
    HashType.UInt64: binary_simple_factory('Q'),
    HashType.Float: binary_simple_factory('f'),
    HashType.Double: binary_simple_factory('d'),

    HashType.VectorBool: partial(yield_binary_numpy_vector, numpy=np.bool_),
    HashType.VectorChar: yield_binary_vector_char,
    HashType.VectorInt8: partial(yield_binary_numpy_vector, numpy=np.int8),
    HashType.VectorUInt8: partial(yield_binary_numpy_vector, numpy=np.uint8),
    HashType.VectorInt16: partial(yield_binary_numpy_vector, numpy=np.int16),
    HashType.VectorUInt16: partial(yield_binary_numpy_vector, numpy=np.uint16),
    HashType.VectorInt32: partial(yield_binary_numpy_vector, numpy=np.int32),
    HashType.VectorUInt32: partial(yield_binary_numpy_vector, numpy=np.uint32),
    HashType.VectorInt64: partial(yield_binary_numpy_vector, numpy=np.int64),
    HashType.VectorUInt64: partial(yield_binary_numpy_vector, numpy=np.uint64),
    HashType.VectorFloat: partial(yield_binary_numpy_vector, numpy=np.float32),
    HashType.VectorDouble: partial(yield_binary_numpy_vector,
                                   numpy=np.float64),

    HashType.String: yield_binary_string,
    HashType.VectorString: yield_binary_vector_string,
    HashType.Hash: yield_binary_hash,
    HashType.VectorHash: yield_binary_vector_hash,
    HashType.Schema: yield_binary_schema,
    HashType.None_: yield_binary_empty,
    HashType.ByteArray: yield_binary_bytearray,
}.items()}
