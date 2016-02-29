from enum import Enum


class HashType(Enum):
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
