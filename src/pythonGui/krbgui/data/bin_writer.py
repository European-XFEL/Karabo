from functools import partial
from io import BytesIO
from struct import pack


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
        type_name = attrs.pop('KRB_Type')
        attr_count = len(attrs)
        type_num, writer = WRITER_MAP[type_name]

        write_key(fp, key)
        write_simple(fp, type_num, fmt='I')
        write_simple(fp, attr_count, fmt='I')
        for akey, avalue in attrs.items():
            atype_name = attr_types[akey]
            atype_num, awriter = WRITER_MAP[atype_name]
            write_key(fp, akey)
            write_simple(fp, atype_num, fmt='I')
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
    mem_file = BytesIO()
    write_hash(mem_file, sch.hash)
    buffer = mem_file.getvalue()
    key = sch.name.encode('utf8')

    write_simple(fp, len(buffer) + len(key) + 1, fmt='I')
    write_key(fp, key)
    fp.write(buffer)


def write_simple(fp, obj, fmt=None):
    fp.write(pack(fmt, obj))


def write_string(fp, obj):
    write_simple(fp, len(obj), fmt='I')
    fp.write(obj.encode('utf8'))


WRITER_MAP = {
    'Bool': (0, partial(write_simple, fmt='?')),
    'BoolArray': (1, write_array),
    'Char': (2, partial(write_simple, fmt='c')),
    'Bytes': (3, write_bytes),
    'Int8': (4, partial(write_simple, fmt='b')),
    'Int8Array': (5, write_array),
    'UInt8': (6, partial(write_simple, fmt='B')),
    'UInt8Array': (7, write_array),
    'Int16': (8, partial(write_simple, fmt='h')),
    'Int16Array': (9, write_array),
    'UInt16': (10, partial(write_simple, fmt='H')),
    'UInt16Array': (11, write_array),
    'Int32': (12, partial(write_simple, fmt='i')),
    'Int32Array': (13, write_array),
    'UInt32': (14, partial(write_simple, fmt='I')),
    'UInt32Array': (15, write_array),
    'Int64': (16, partial(write_simple, fmt='q')),
    'Int64Array': (17, write_array),
    'UInt64': (18, partial(write_simple, fmt='Q')),
    'UInt64Array': (19, write_array),
    'Float32': (20, partial(write_simple, fmt='f')),
    'Float32Array': (21, write_array),
    'Float64': (22, partial(write_simple, fmt='d')),
    'Float64Array': (23, write_array),
    'Complex64': (24, partial(write_complex, fmt='ff')),
    'Complex64Array': (25, write_array),
    'Complex128': (26, partial(write_complex, fmt='dd')),
    'Complex128Array': (27, write_array),
    'String': (28, write_string),
    'StringList': (29, partial(write_list, writer=write_string)),
    'Hash': (30, write_hash),
    'HashList': (31, partial(write_list, writer=write_hash)),
    'Schema': (47, write_schema),
    'None': (50, write_empty),
}
