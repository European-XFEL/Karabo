from functools import partial
from io import BytesIO
from struct import pack

from .typenums import HashType


def write_binary_hash(hsh):
    """ Convert a Hash to a string of bytes.
    """
    fp = BytesIO()
    with fp:
        write_hash(fp, hsh)
        return fp.getvalue()


def write_binary_schema(schema):
    """ Convert a Schema to a string of bytes.
    """
    fp = BytesIO()
    with fp:
        write_schema(fp, schema)
        return fp.getvalue()


#######################
# Writer implementation

def write_array(fp, arr):
    write_simple(fp, len(arr), fmt='I')
    fp.write(arr.tostring())


def write_bytes(fp, obj):
    write_simple(fp, len(obj), fmt='I')
    fp.write(obj)


def write_complex(fp, obj, fmt=None):
    fp.write(pack(fmt, obj.real, obj.imag))


def write_empty(fp, *args):
    write_simple(fp, 0, fmt='I')


def write_hash(fp, hsh):
    member_count = len(hsh)
    write_simple(fp, member_count, fmt='I')
    for key, value, attrs in hsh.iterall():
        attr_types = attrs.pop('KRB_AttrTypes')
        type_enum = attrs.pop('KRB_Type')
        attr_count = len(attrs)
        writer = __WRITER_MAP[type_enum]

        write_key(fp, key)
        write_simple(fp, type_enum.value, fmt='I')
        write_simple(fp, attr_count, fmt='I')
        for akey, avalue in attrs.items():
            atype_enum = attr_types[akey]
            awriter = __WRITER_MAP[atype_enum]
            write_key(fp, akey)
            write_simple(fp, atype_enum.value, fmt='I')
            awriter(fp, avalue)

        writer(fp, value)


def write_key(fp, key):
    write_simple(fp, len(key), fmt='B')
    buf = key.encode('ascii') if hasattr(key, 'encode') else key
    fp.write(buf)


def write_list(fp, lst, writer=None):
    write_simple(fp, len(lst), fmt='I')
    for obj in lst:
        writer(fp, obj)


def write_schema(fp, sch):
    buffer = write_binary_hash(sch.hash)
    key = sch.name.encode('utf8')

    write_simple(fp, len(buffer) + len(key) + 1, fmt='I')
    write_key(fp, key)
    fp.write(buffer)


def write_simple(fp, obj, fmt=None):
    fp.write(pack(fmt, obj))


def write_string(fp, obj):
    write_simple(fp, len(obj), fmt='I')
    fp.write(obj.encode('utf8'))


__WRITER_MAP = {
    HashType.Bool: partial(write_simple, fmt='?'),
    HashType.BoolArray: write_array,
    HashType.Char: partial(write_simple, fmt='c'),
    HashType.Bytes: write_bytes,
    HashType.Int8: partial(write_simple, fmt='b'),
    HashType.Int8Array: write_array,
    HashType.UInt8: partial(write_simple, fmt='B'),
    HashType.UInt8Array: write_array,
    HashType.Int16: partial(write_simple, fmt='h'),
    HashType.Int16Array: write_array,
    HashType.UInt16: partial(write_simple, fmt='H'),
    HashType.UInt16Array: write_array,
    HashType.Int32: partial(write_simple, fmt='i'),
    HashType.Int32Array: write_array,
    HashType.UInt32: partial(write_simple, fmt='I'),
    HashType.UInt32Array: write_array,
    HashType.Int64: partial(write_simple, fmt='q'),
    HashType.Int64Array: write_array,
    HashType.UInt64: partial(write_simple, fmt='Q'),
    HashType.UInt64Array: write_array,
    HashType.Float32: partial(write_simple, fmt='f'),
    HashType.Float32Array: write_array,
    HashType.Float64: partial(write_simple, fmt='d'),
    HashType.Float64Array: write_array,
    HashType.Complex64: partial(write_complex, fmt='ff'),
    HashType.Complex64Array: write_array,
    HashType.Complex128: partial(write_complex, fmt='dd'),
    HashType.Complex128Array: write_array,
    HashType.String: write_string,
    HashType.StringList: partial(write_list, writer=write_string),
    HashType.Hash: write_hash,
    HashType.HashList: partial(write_list, writer=write_hash),
    HashType.Schema: write_schema,
    HashType.None_: write_empty,
}
