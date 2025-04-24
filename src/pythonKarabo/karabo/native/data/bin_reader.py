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
from struct import calcsize, unpack

import numpy as np

from .hash import Hash, HashByte, HashElement, HashList
from .schema import Schema
from .typenums import HashType


class BinaryParser:
    def __init__(self):
        self.pos = 0
        self.data = None

    def readFormat(self, fmt):
        fmt = fmt.encode("ascii")
        size = calcsize(fmt)
        self.pos += size
        return unpack(fmt, self.data[self.pos - size:self.pos])

    def readKey(self):
        size, = unpack('B', self.data[self.pos:self.pos + 1])
        self.pos += size + 1
        return self.data[self.pos - size:self.pos].decode("ascii")

    def read(self, data):
        self.pos = 0
        self.data = data
        return read_binary_hash(self)


def decodeBinary(data: bytes):
    """Decode a binary hash object `data` and return a Hash"""
    parser = BinaryParser()
    return parser.read(data)


def decodeBinaryPos(data: bytes):
    """Decode a binary hash object `data`

    This function will return a decoded Hash as well as the position of the
    parser in a tuple.
    """
    parser = BinaryParser()
    return parser.read(data), parser.pos


def read_binary_string(file):
    size, = file.readFormat('I')
    file.pos += size
    return str(file.data[file.pos - size:file.pos], 'utf8')


def read_binary_hash(file):
    size, = file.readFormat('I')
    ret = Hash()
    for _ in range(size):
        key = file.readKey()
        hash_type, = file.readFormat('I')
        type_reader = __QREADER[hash_type]
        asize, = file.readFormat('I')
        attrs = {}
        for _ in range(asize):
            akey = file.readKey()
            attr_type, = file.readFormat('I')
            attr_reader = __QREADER[attr_type]
            attrs[akey] = attr_reader(file)
        # Optimization: Set value and attributes simultaneously
        ret._setelement(key, HashElement(type_reader(file), attrs))
        if hash_type == 31 and key == 'KRB_Sequence':
            # Special case: This is the equivalent of what is done by the
            # C++ binary serializer, HashBinarySerializer, in its method
            # 'load(vector<Hash>&, const char*, const size_t)'
            ret = ret['KRB_Sequence']
    return ret


def read_binary_schema(file):
    lsize, = file.readFormat('I')  # ignore length
    op = file.pos
    size, = file.readFormat('B')
    name = str(file.data[file.pos:file.pos + size], "utf8")
    file.pos += size
    ret = read_binary_hash(file)
    assert file.pos - op == lsize, f'failed: {file.pos} {op} {lsize}'
    return Schema(name, hash=ret)


def read_binary_numpy_vector(file, numpy=None):
    assert numpy is not None
    size, = file.readFormat('I')
    ret = np.frombuffer(file.data, numpy, size, file.pos)
    file.pos += numpy().itemsize * size
    return ret


def read_binary_numpy(file, numpy=None):
    ret = np.frombuffer(file.data, numpy, 1, file.pos)[0]
    file.pos += numpy().itemsize
    return ret


def read_binary_bool(file, numpy=np.int8):
    """The binary bool with a fixed 'numpy'"""
    ret = bool(np.frombuffer(file.data, numpy, 1, file.pos)[0])
    file.pos += numpy().itemsize
    return ret


def read_binary_char(file):
    file.pos += 1
    return HashByte(file.data[file.pos - 1:file.pos].decode("ascii"))


def read_binary_string_list(file):
    size, = file.readFormat('I')
    return [read_binary_string(file) for _ in range(size)]


def read_vector_hash(file):
    size, = file.readFormat('I')
    return HashList(read_binary_hash(file) for _ in range(size))


def read_binary_bytearray(file):
    size, = file.readFormat('I')
    file.pos += size
    return bytearray(file.data[file.pos - size:file.pos])


def read_binary_vector_char(file):
    size, = file.readFormat('I')
    file.pos += size
    return bytes(file.data[file.pos - size:file.pos])


def read_binary_empty(file):
    file.readFormat('I')  # ignore length
    return None


__READER_MAP = {
    HashType.Bool: read_binary_bool,
    HashType.Char: read_binary_char,
    HashType.Int8: partial(read_binary_numpy, numpy=np.int8),
    HashType.UInt8: partial(read_binary_numpy, numpy=np.uint8),
    HashType.Int16: partial(read_binary_numpy, numpy=np.int16),
    HashType.UInt16: partial(read_binary_numpy, numpy=np.uint16),
    HashType.Int32: partial(read_binary_numpy, numpy=np.int32),
    HashType.UInt32: partial(read_binary_numpy, numpy=np.uint32),
    HashType.Int64: partial(read_binary_numpy, numpy=np.int64),
    HashType.UInt64: partial(read_binary_numpy, numpy=np.uint64),
    HashType.Float: partial(read_binary_numpy, numpy=np.float32),
    HashType.Double: partial(read_binary_numpy, numpy=np.float64),

    HashType.VectorBool: partial(read_binary_numpy_vector, numpy=np.bool_),
    HashType.VectorChar: read_binary_vector_char,
    HashType.VectorInt8: partial(read_binary_numpy_vector, numpy=np.int8),
    HashType.VectorUInt8: partial(read_binary_numpy_vector, numpy=np.uint8),
    HashType.VectorInt16: partial(read_binary_numpy_vector, numpy=np.int16),
    HashType.VectorUInt16: partial(read_binary_numpy_vector, numpy=np.uint16),
    HashType.VectorInt32: partial(read_binary_numpy_vector, numpy=np.int32),
    HashType.VectorUInt32: partial(read_binary_numpy_vector, numpy=np.uint32),
    HashType.VectorInt64: partial(read_binary_numpy_vector, numpy=np.int64),
    HashType.VectorUInt64: partial(read_binary_numpy_vector, numpy=np.uint64),
    HashType.VectorFloat: partial(read_binary_numpy_vector, numpy=np.float32),
    HashType.VectorDouble: partial(read_binary_numpy_vector, numpy=np.float64),

    HashType.String: read_binary_string,
    HashType.VectorString: read_binary_string_list,
    HashType.Hash: read_binary_hash,
    HashType.VectorHash: read_vector_hash,
    HashType.Schema: read_binary_schema,
    HashType.None_: read_binary_empty,
    HashType.ByteArray: read_binary_bytearray,
}

__QREADER = {k.value: v for k, v in __READER_MAP.items()}
