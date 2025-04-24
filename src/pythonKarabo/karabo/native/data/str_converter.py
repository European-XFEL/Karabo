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
import ast
import base64
from functools import partial
from json import dumps

import numpy as np

from .hash import HashList, get_hash_type_from_data
from .schema import Schema
from .typenums import HashType
from .utils import dictToHash, hashToDict
from .xml_reader import decodeXML
from .xml_writer import yield_xml_hash


def string_from_hashtype(data):
    """Convert a given hashtype `data` to a string

    Note: Hash, Schema and VectorHash take XML serialization
    """
    hashtype = get_hash_type_from_data(data)
    converter = _STRING_CONVERTER[hashtype]

    return converter(data)


def string_from_simple(data):
    return str(data)


def string_from_hash(data):
    return dumps(hashToDict(data))


def string_from_vector_hash(data):
    return dumps([hashToDict(h) for h in data])


def string_from_numpy_vector(data):
    return ",".join(str(x) for x in data)


def string_from_vector_char(data):
    return str(base64.b64encode(data).decode("ascii"))


def string_from_bytearray(data):
    return str(base64.b64encode(data).decode("ascii"))


def string_from_vector_bool(data):
    return ",".join(str(int(i)) for i in data)


def string_from_list(data):
    return ",".join(str(x) for x in data)


def string_from_schema(data):
    return data.name + ":" + "".join(yield_xml_hash(data.hash))


def string_from_bool(data):
    return '1' if data else '0'


_STRING_CONVERTER = {
    HashType.Bool: string_from_bool,
    HashType.Char: string_from_simple,
    HashType.Int8: string_from_simple,
    HashType.Int16: string_from_simple,
    HashType.Int32: string_from_simple,
    HashType.Int64: string_from_simple,
    HashType.UInt8: string_from_simple,
    HashType.UInt16: string_from_simple,
    HashType.UInt32: string_from_simple,
    HashType.UInt64: string_from_simple,
    HashType.Float: string_from_simple,
    HashType.Double: string_from_simple,

    HashType.VectorBool: string_from_vector_bool,
    HashType.VectorChar: string_from_vector_char,
    HashType.VectorInt8: string_from_numpy_vector,
    HashType.VectorInt16: string_from_numpy_vector,
    HashType.VectorInt32: string_from_numpy_vector,
    HashType.VectorInt64: string_from_numpy_vector,
    HashType.VectorUInt8: string_from_numpy_vector,
    HashType.VectorUInt16: string_from_numpy_vector,
    HashType.VectorUInt32: string_from_numpy_vector,
    HashType.VectorUInt64: string_from_numpy_vector,
    HashType.VectorFloat: string_from_numpy_vector,
    HashType.VectorDouble: string_from_numpy_vector,

    HashType.Hash: string_from_hash,
    HashType.VectorHash: string_from_vector_hash,
    HashType.String: string_from_simple,
    HashType.VectorString: string_from_list,
    HashType.Schema: string_from_schema,
    HashType.None_: string_from_simple,
    HashType.ByteArray: string_from_bytearray,
}


# --------------- HashType Conversion ---------------
#


def hashtype_from_string(hash_type, data):
    """Convert a given `data` string to the equivalent value of `hash_type`

    :param hashtype: The hashtype declaration. Can be the integer number
                     or the `HashType` enum
    :param data: The data string
    """
    assert type(data) is str, f"{type(data)}"

    hash_type = HashType(hash_type)
    converter = _HASHTYPE_CONVERTER[hash_type]

    return converter(data)


def bool_from_string(s):
    return bool(int(s))


def pass_from_string(s):
    return s


def none_from_string(s):
    assert s == "" or s == "None"
    return None


def number_from_string(s, numpy=None):
    return numpy(s)


def vector_char_from_string(s):
    return base64.b64decode(s)


def numpy_vector_from_string(s, numpy=None):
    if s:
        return np.fromstring(s, sep=",", dtype=numpy)
    else:
        return np.array([], dtype=numpy)


def hash_from_string(s):
    """Convert a dictionary form from string to a Hash"""
    return dictToHash(ast.literal_eval(s))


def vector_hash_from_string(s):
    if s:
        dicts = ast.literal_eval(s)
        return HashList(dictToHash(d) for d in dicts)
    else:
        return HashList([])


def list_from_string(s):
    if not s:
        return []
    return [ss.strip() for ss in s.split(',')]


def schema_from_string(s):
    name, xml = s.split(":", 1)
    return Schema(name, hash=decodeXML(xml))


def bytearray_from_string(s):
    return bytearray(base64.b64decode(s))


_HASHTYPE_CONVERTER = {
    HashType.Bool: bool_from_string,
    HashType.Char: pass_from_string,
    HashType.Int8: partial(number_from_string, numpy=np.int8),
    HashType.Int16: partial(number_from_string, numpy=np.int16),
    HashType.Int32: partial(number_from_string, numpy=np.int32),
    HashType.Int64: partial(number_from_string, numpy=np.int64),
    HashType.UInt8: partial(number_from_string, numpy=np.uint8),
    HashType.UInt16: partial(number_from_string, numpy=np.uint16),
    HashType.UInt32: partial(number_from_string, numpy=np.uint32),
    HashType.UInt64: partial(number_from_string, numpy=np.uint64),
    HashType.Float: partial(number_from_string, numpy=np.float32),
    HashType.Double: partial(number_from_string, numpy=np.float64),

    HashType.VectorBool: partial(numpy_vector_from_string, numpy=np.bool_),
    HashType.VectorChar: vector_char_from_string,
    HashType.VectorInt8: partial(numpy_vector_from_string, numpy=np.int8),
    HashType.VectorInt16: partial(numpy_vector_from_string, numpy=np.int16),
    HashType.VectorInt32: partial(numpy_vector_from_string, numpy=np.int32),
    HashType.VectorInt64: partial(numpy_vector_from_string, numpy=np.int64),
    HashType.VectorUInt8: partial(numpy_vector_from_string, numpy=np.uint8),
    HashType.VectorUInt16: partial(numpy_vector_from_string, numpy=np.uint16),
    HashType.VectorUInt32: partial(numpy_vector_from_string, numpy=np.uint32),
    HashType.VectorUInt64: partial(numpy_vector_from_string, numpy=np.uint64),
    HashType.VectorFloat: partial(numpy_vector_from_string, numpy=np.float32),
    HashType.VectorDouble: partial(numpy_vector_from_string, numpy=np.float64),

    HashType.Hash: hash_from_string,
    HashType.VectorHash: vector_hash_from_string,
    HashType.String: pass_from_string,
    HashType.VectorString: list_from_string,
    HashType.Schema: schema_from_string,
    HashType.None_: none_from_string,
    HashType.ByteArray: bytearray_from_string,
}
