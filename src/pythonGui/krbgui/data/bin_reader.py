from functools import partial
from struct import calcsize, unpack

import numpy as np
from .hash import Hash, Schema
from .typenums import HashType


class _IndexedBuffer(object):
    """ A simple wrapper for byte buffers to make them look like a file that
    can be read.
    """
    def __init__(self, buffer):
        self.buffer = buffer
        self.index = 0

    def read(self, size):
        start = self.index
        self.index = end = start + size
        return self.buffer[start:end]


def read_binary_hash(buffer):
    """ Read a Hash from a byte buffer.
    """
    fp = _IndexedBuffer(buffer)
    return read_hash(fp)


def read_binary_schema(buffer):
    """ Read a Schema from a byte buffer.
    """
    fp = _IndexedBuffer(buffer)
    return read_schema(fp)


#######################
# Reader implementation

def read_array(fp, dtype=None):
    count = read_simple(fp, fmt='I')
    size = count * dtype().itemsize
    buffer = fp.read(size)
    return np.frombuffer(buffer, dtype=dtype, count=count)


def read_bytes(fp):
    size = read_simple(fp, fmt='I')
    return bytes(fp.read(size))


def read_complex(fp, fmt=None):
    return complex(*unpack(fmt, fp.read(calcsize(fmt))))


def read_empty(fp):
    read_simple(fp, fmt='I')
    return None


def read_hash(fp):
    member_count = read_simple(fp, fmt='I')
    hsh = Hash()
    for i in range(member_count):
        key = read_key(fp)
        type_enum = HashType(read_simple(fp, fmt='I'))
        reader = __READER_MAP[type_enum]

        attr_count = read_simple(fp, fmt='I')
        attr_types = {}
        attrs = {'KRB_Type': type_enum.name, 'KRB_AttrTypes': attr_types}
        for j in range(attr_count):
            akey = read_key(fp)
            atype_enum = HashType(read_simple(fp, fmt='I'))
            areader = __READER_MAP[atype_enum]
            attr_types[akey] = atype_enum.name
            attrs[akey] = areader(fp)

        hsh[key] = reader(fp)
        hsh[key, ...] = attrs

    return hsh


def read_key(fp):
    size = read_simple(fp, fmt='B')
    return str(fp.read(size), 'ascii')


def read_list(fp, reader=None):
    size = read_simple(fp, fmt='I')
    return [reader(fp) for i in range(size)]


def read_schema(fp):
    read_simple(fp, fmt='I')  # ignore length
    key = read_key(fp)
    ret = read_hash(fp)
    return Schema(key, hash=ret)


def read_simple(fp, fmt=None):
    size = calcsize(fmt)
    return unpack(fmt, fp.read(size))[0]


def read_string(fp):
    size = read_simple(fp, fmt='I')
    return str(fp.read(size), 'utf8')


__READER_MAP = {
    HashType.Bool: partial(read_simple, fmt='?'),
    HashType.BoolArray: partial(read_array, dtype=np.bool_),
    HashType.Char: partial(read_simple, fmt='c'),
    HashType.Bytes: read_bytes,
    HashType.Int8: partial(read_simple, fmt='b'),
    HashType.Int8Array: partial(read_array, dtype=np.int8),
    HashType.UInt8: partial(read_simple, fmt='B'),
    HashType.UInt8Array: partial(read_array, dtype=np.uint8),
    HashType.Int16: partial(read_simple, fmt='h'),
    HashType.Int16Array: partial(read_array, dtype=np.int16),
    HashType.UInt16: partial(read_simple, fmt='H'),
    HashType.UInt16Array: partial(read_array, dtype=np.uint16),
    HashType.Int32: partial(read_simple, fmt='i'),
    HashType.Int32Array: partial(read_array, dtype=np.int32),
    HashType.UInt32: partial(read_simple, fmt='I'),
    HashType.UInt32Array: partial(read_array, dtype=np.uint32),
    HashType.Int64: partial(read_simple, fmt='q'),
    HashType.Int64Array: partial(read_array, dtype=np.int64),
    HashType.UInt64: partial(read_simple, fmt='Q'),
    HashType.UInt64Array: partial(read_array, dtype=np.uint64),
    HashType.Float32: partial(read_simple, fmt='f'),
    HashType.Float32Array: partial(read_array, dtype=np.float32),
    HashType.Float64: partial(read_simple, fmt='d'),
    HashType.Float64Array: partial(read_array, dtype=np.float64),
    HashType.Complex64: partial(read_complex, fmt='ff'),
    HashType.Complex64Array: partial(read_array, dtype=np.complex64),
    HashType.Complex128: partial(read_complex, fmt='dd'),
    HashType.Complex128Array: partial(read_array, dtype=np.complex128),
    HashType.String: read_string,
    HashType.StringList: partial(read_list, reader=read_string),
    HashType.Hash: read_hash,
    HashType.HashList: partial(read_list, reader=read_hash),
    HashType.Schema: read_schema,
    HashType.None_: read_empty,
}
