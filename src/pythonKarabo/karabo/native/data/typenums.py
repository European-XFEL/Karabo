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
from enum import Enum


class HashType(Enum):
    """ This enumeration matches types defined in the Karabo code base."""
    Bool = 0
    VectorBool = 1
    Char = 2
    VectorChar = 3
    Int8 = 4
    VectorInt8 = 5
    UInt8 = 6
    VectorUInt8 = 7
    Int16 = 8
    VectorInt16 = 9
    UInt16 = 10
    VectorUInt16 = 11
    Int32 = 12
    VectorInt32 = 13
    UInt32 = 14
    VectorUInt32 = 15
    Int64 = 16
    VectorInt64 = 17
    UInt64 = 18
    VectorUInt64 = 19
    Float = 20
    VectorFloat = 21
    Double = 22
    VectorDouble = 23
    String = 28
    VectorString = 29
    Hash = 30
    VectorHash = 31
    Schema = 32
    None_ = 35
    ByteArray = 37


HASH_TYPE_TO_XML_TYPE = {
    HashType.Bool: 'BOOL',
    HashType.VectorBool: 'VECTOR_BOOL',
    HashType.Char: 'CHAR',
    HashType.VectorChar: 'VECTOR_CHAR',
    HashType.Int8: 'INT8',
    HashType.VectorInt8: 'VECTOR_INT8',
    HashType.UInt8: 'UINT8',
    HashType.VectorUInt8: 'VECTOR_UINT8',
    HashType.Int16: 'INT16',
    HashType.VectorInt16: 'VECTOR_INT16',
    HashType.UInt16: 'UINT16',
    HashType.VectorUInt16: 'VECTOR_UINT16',
    HashType.Int32: 'INT32',
    HashType.VectorInt32: 'VECTOR_INT32',
    HashType.UInt32: 'UINT32',
    HashType.VectorUInt32: 'VECTOR_UINT32',
    HashType.Int64: 'INT64',
    HashType.VectorInt64: 'VECTOR_INT64',
    HashType.UInt64: 'UINT64',
    HashType.VectorUInt64: 'VECTOR_UINT64',
    HashType.Float: 'FLOAT',
    HashType.VectorFloat: 'VECTOR_FLOAT',
    HashType.Double: 'DOUBLE',
    HashType.VectorDouble: 'VECTOR_DOUBLE',
    HashType.String: 'STRING',
    HashType.VectorString: 'VECTOR_STRING',
    HashType.Hash: 'HASH',
    HashType.VectorHash: 'VECTOR_HASH',
    HashType.ByteArray: 'BYTE_ARRAY',
    HashType.Schema: 'SCHEMA',
    HashType.None_: 'NONE',
}

XML_TYPE_TO_HASH_TYPE = {v: k for k, v in HASH_TYPE_TO_XML_TYPE.items()}
