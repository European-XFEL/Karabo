from functools import partial
from struct import calcsize, unpack

import numpy as np
from .hash import Hash, Schema


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
        type_num = read_simple(fp, fmt='I')
        type_name, reader = READER_MAP[type_num]

        attr_count = read_simple(fp, fmt='I')
        attr_types = {}
        attrs = {'KRB_Type': type_name, 'KRB_AttrTypes': attr_types}
        for j in range(attr_count):
            akey = read_key(fp)
            atype_num = read_simple(fp, fmt='I')
            atype_name, areader = READER_MAP[atype_num]
            attr_types[akey] = atype_name
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


READER_MAP = {
    0: ('Bool', partial(read_simple, fmt='?')),
    1: ('BoolArray', partial(read_array, dtype=np.bool_)),
    2: ('Char', partial(read_simple, fmt='c')),
    3: ('Bytes', read_bytes),
    4: ('Int8', partial(read_simple, fmt='b')),
    5: ('Int8Array', partial(read_array, dtype=np.int8)),
    6: ('UInt8', partial(read_simple, fmt='B')),
    7: ('UInt8Array', partial(read_array, dtype=np.uint8)),
    8: ('Int16', partial(read_simple, fmt='h')),
    9: ('Int16Array', partial(read_array, dtype=np.int16)),
    10: ('UInt16', partial(read_simple, fmt='H')),
    11: ('UInt16Array', partial(read_array, dtype=np.uint16)),
    12: ('Int32', partial(read_simple, fmt='i')),
    13: ('Int32Array', partial(read_array, dtype=np.int32)),
    14: ('UInt32', partial(read_simple, fmt='I')),
    15: ('UInt32Array', partial(read_array, dtype=np.uint32)),
    16: ('Int64', partial(read_simple, fmt='q')),
    17: ('Int64Array', partial(read_array, dtype=np.int64)),
    18: ('UInt64', partial(read_simple, fmt='Q')),
    19: ('UInt64Array', partial(read_array, dtype=np.uint64)),
    20: ('Float32', partial(read_simple, fmt='f')),
    21: ('Float32Array', partial(read_array, dtype=np.float32)),
    22: ('Float64', partial(read_simple, fmt='d')),
    23: ('Float64Array', partial(read_array, dtype=np.float64)),
    24: ('Complex64', partial(read_complex, fmt='ff')),
    25: ('Complex64Array', partial(read_array, dtype=np.complex64)),
    26: ('Complex128', partial(read_complex, fmt='dd')),
    27: ('Complex128Array', partial(read_array, dtype=np.complex128)),
    28: ('String', read_string),
    29: ('StringList', partial(read_list, reader=read_string)),
    30: ('Hash', read_hash),
    31: ('HashList', partial(read_list, reader=read_hash)),
    47: ('Schema', read_schema),
    50: ('None', read_empty),
}
