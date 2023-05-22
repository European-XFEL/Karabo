# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from functools import partial
from struct import pack

import numpy as np

from .enums import NodeType
from .hash import Hash, get_hash_type_from_data
from .typenums import HashType

__all__ = ['encodeBinary', 'writeBinary']


def yieldKey(key):
    key = key.encode('utf8')
    yield pack('B', len(key))
    yield key


def encodeBinary(data):
    """return the binary serialization of hash `data`"""
    hash_type = get_hash_type_from_data(data)
    writer = __WRITER_MAP[hash_type]
    return b"".join(writer(data))


def writeBinary(data, file):
    """write the hash in `data` into `file`"""
    hash_type = get_hash_type_from_data(data)
    writer = __WRITER_MAP[hash_type]
    for d in writer(data):
        file.write(d)


def yield_binary_hash(data):
    yield pack('I', len(data))
    for k, v, attrs in data.iterall():
        yield from yieldKey(k)
        hash_type = get_hash_type_from_data(v)
        type_writer = __WRITER_MAP[hash_type]
        yield pack('II', hash_type.value, len(attrs))
        for ak, av in attrs.items():
            atype = get_hash_type_from_data(av)
            yield from yieldKey(ak)
            yield pack('I', atype.value)
            attr_writer = __WRITER_MAP[atype]
            yield from attr_writer(av)
        yield from type_writer(v)


def yield_binary_schema(data):
    for p in data.hash.paths(intermediate=True):
        nodeType = NodeType(data.hash[p, "nodeType"])
        if nodeType is NodeType.Leaf:
            assert not data.hash[p], f"no proper leaf: {p}"
        else:
            assert isinstance(data.hash[p], Hash), \
                f"no proper node: {p}"
    binary = b''.join(yield_binary_hash(data.hash))
    s = data.name.encode('utf8')
    yield pack('IB', len(binary) + len(s) + 1, len(s))
    yield s
    yield binary


def yield_binary_char(data):
    assert len(data) == 1
    yield data.encode("ascii")


def yield_binary_vector_char(data):
    yield pack('I', len(data))
    yield data


def yield_binary_string(data):
    yield from yield_binary_vector_char(data.encode('utf8'))


def yield_binary_numpy_vector(data, numpy=None):
    if not isinstance(data, np.ndarray) or data.dtype != numpy:
        data = np.array(data, dtype=numpy)
    yield pack('I', len(data))
    yield data.data


def yield_binary_simple(data, fmt=None):
    yield pack(fmt, data)


def yield_binary_bool(data):
    yield pack('b', 1 if data else 0)


def yield_binary_complex(data, fmt=None):
    yield pack(fmt, data.real, data.imag)


def yield_binary_vector_hash(data):
    yield pack('I', len(data))
    for d in data:
        yield from yield_binary_hash(d)


def yield_binary_vector_string(data):
    yield pack('I', len(data))
    for d in data:
        yield from yield_binary_string(d)


def yield_binary_empty(data):
    yield pack('I', 0)


def yield_binary_bytearray(data):
    mv = memoryview(data)
    yield pack('I', mv.nbytes)
    yield mv


__WRITER_MAP = {
    HashType.Bool: yield_binary_bool,
    HashType.Char: yield_binary_char,
    HashType.Int8: partial(yield_binary_simple, fmt='b'),
    HashType.UInt8: partial(yield_binary_simple, fmt='B'),
    HashType.Int16: partial(yield_binary_simple, fmt='h'),
    HashType.UInt16: partial(yield_binary_simple, fmt='H'),
    HashType.Int32: partial(yield_binary_simple, fmt='i'),
    HashType.UInt32: partial(yield_binary_simple, fmt='I'),
    HashType.Int64: partial(yield_binary_simple, fmt='q'),
    HashType.UInt64: partial(yield_binary_simple, fmt='Q'),
    HashType.Float: partial(yield_binary_simple, fmt='f'),
    HashType.Double: partial(yield_binary_simple, fmt='d'),
    HashType.ComplexFloat: partial(yield_binary_complex, fmt='ff'),
    HashType.ComplexDouble: partial(yield_binary_complex, fmt='dd'),

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
    HashType.VectorComplexFloat: partial(yield_binary_numpy_vector,
                                         numpy=np.complex64),
    HashType.VectorComplexDouble: partial(yield_binary_numpy_vector,
                                          numpy=np.complex128),

    HashType.String: yield_binary_string,
    HashType.VectorString: yield_binary_vector_string,
    HashType.Hash: yield_binary_hash,
    HashType.VectorHash: yield_binary_vector_hash,
    HashType.Schema: yield_binary_schema,
    HashType.None_: yield_binary_empty,
    HashType.ByteArray: yield_binary_bytearray,
}
