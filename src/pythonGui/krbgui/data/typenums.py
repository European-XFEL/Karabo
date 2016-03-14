from enum import Enum


class HashType(Enum):
    """ This enumeration matches types defined in the Karabo C++ code base.

    The serialized binary format of Hash objects uses these numbers to denote
    types of items.
    """
    Bool = 0
    BoolArray = 1
    Char = 2
    Bytes = 3
    Int8 = 4
    Int8Array = 5
    UInt8 = 6
    UInt8Array = 7
    Int16 = 8
    Int16Array = 9
    UInt16 = 10
    UInt16Array = 11
    Int32 = 12
    Int32Array = 13
    UInt32 = 14
    UInt32Array = 15
    Int64 = 16
    Int64Array = 17
    UInt64 = 18
    UInt64Array = 19
    Float32 = 20
    Float32Array = 21
    Float64 = 22
    Float64Array = 23
    Complex64 = 24
    Complex64Array = 25
    Complex128 = 26
    Complex128Array = 27
    String = 28
    StringList = 29
    Hash = 30
    HashList = 31
    Schema = 47
    None_ = 50


HASH_TYPE_TO_XML_TYPE = {
    HashType.Bool: 'BOOL',
    HashType.BoolArray: 'VECTOR_BOOL',
    HashType.Char: 'CHAR',
    HashType.Bytes: 'VECTOR_CHAR',
    HashType.Int8: 'INT8',
    HashType.Int8Array: 'VECTOR_INT8',
    HashType.UInt8: 'UINT8',
    HashType.UInt8Array: 'VECTOR_UINT8',
    HashType.Int16: 'INT16',
    HashType.Int16Array: 'VECTOR_INT16',
    HashType.UInt16: 'UINT16',
    HashType.UInt16Array: 'VECTOR_UINT16',
    HashType.Int32: 'INT32',
    HashType.Int32Array: 'VECTOR_INT32',
    HashType.UInt32: 'UINT32',
    HashType.UInt32Array: 'VECTOR_UINT32',
    HashType.Int64: 'INT64',
    HashType.Int64Array: 'VECTOR_INT64',
    HashType.UInt64: 'UINT64',
    HashType.UInt64Array: 'VECTOR_UINT64',
    HashType.Float32: 'FLOAT',
    HashType.Float32Array: 'VECTOR_FLOAT',
    HashType.Float64: 'DOUBLE',
    HashType.Float64Array: 'VECTOR_DOUBLE',
    HashType.Complex64: 'COMPLEX_FLOAT',
    HashType.Complex64Array: 'VECTOR_COMPLEX_FLOAT',
    HashType.Complex128: 'COMPLEX_DOUBLE',
    HashType.Complex128Array: 'VECTOR_COMPLEX_DOUBLE',
    HashType.String: 'STRING',
    HashType.StringList: 'VECTOR_STRING',
    HashType.Hash: 'HASH',
    HashType.HashList: 'VECTOR_HASH',
    HashType.Schema: 'SCHEMA',
    HashType.None_: 'NONE',
}
HASH_TYPE_TO_XML_ATTR_TYPE = {k: 'KRB_' + v
                              for k, v in HASH_TYPE_TO_XML_TYPE.items()}
XML_TYPE_TO_HASH_TYPE = {v: k for k, v in HASH_TYPE_TO_XML_TYPE.items()}
XML_ATTR_TYPE_TO_HASH_TYPE = {v: k
                              for k, v in HASH_TYPE_TO_XML_ATTR_TYPE.items()}
