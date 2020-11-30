import base64

from .xml_writer import yield_xml_hash, yield_xml_vector_hash
from .hash import get_hash_type_from_data
from .typenums import HashType

__all__ = ['convert_string_hashtype']


def convert_string_hashtype(data):
    """Convert a given data with to a string

    Note: Hash, Schema and VectorHash take XML serialization
    """
    hashtype = get_hash_type_from_data(data)
    converter = _CONVERTER[hashtype]

    return converter(data)


def get_string_simple(data):
    return str(data)


def get_string_hash(data):
    return "".join(yield_xml_hash(data))


def get_string_vector_hash(data):
    return "".join(yield_xml_vector_hash(data))


def get_string_numpy_vector(data):
    return ",".join(str(x) for x in data)


def get_string_vector_char(data):
    return str(base64.b64encode(data).decode("ascii"))


def get_string_byte_array(data):
    return str(base64.b64encode(data).decode("ascii"))


def get_string_vector_bool(data):
    return ",".join(str(int(i)) for i in data)


def get_string_list(data):
    return ",".join(str(x) for x in data)


def get_string_schema(data):
    return data.name + ":" + "".join(yield_xml_hash(data.hash))


def get_string_bool(data):
    return '1' if data else '0'


def get_string_complex(data):
    return "({},{})".format(data.real, data.imag)


_CONVERTER = {
    HashType.Bool: get_string_bool,
    HashType.Char: get_string_simple,
    HashType.Int8: get_string_simple,
    HashType.Int16: get_string_simple,
    HashType.Int32: get_string_simple,
    HashType.Int64: get_string_simple,
    HashType.UInt8: get_string_simple,
    HashType.UInt16: get_string_simple,
    HashType.UInt32: get_string_simple,
    HashType.UInt64: get_string_simple,
    HashType.Float: get_string_simple,
    HashType.Double: get_string_simple,
    HashType.ComplexFloat: get_string_complex,
    HashType.ComplexDouble: get_string_complex,

    HashType.VectorBool: get_string_vector_bool,
    HashType.VectorChar: get_string_vector_char,
    HashType.VectorInt8: get_string_numpy_vector,
    HashType.VectorInt16: get_string_numpy_vector,
    HashType.VectorInt32: get_string_numpy_vector,
    HashType.VectorInt64: get_string_numpy_vector,
    HashType.VectorUInt8: get_string_numpy_vector,
    HashType.VectorUInt16: get_string_numpy_vector,
    HashType.VectorUInt32: get_string_numpy_vector,
    HashType.VectorUInt64: get_string_numpy_vector,
    HashType.VectorFloat: get_string_numpy_vector,
    HashType.VectorDouble: get_string_numpy_vector,
    HashType.VectorComplexFloat: get_string_numpy_vector,
    HashType.VectorComplexDouble: get_string_numpy_vector,

    HashType.Hash: get_string_hash,
    HashType.VectorHash: get_string_vector_hash,
    HashType.String: get_string_simple,
    HashType.VectorString: get_string_list,
    HashType.Schema: get_string_schema,
    HashType.None_: get_string_simple,
    HashType.ByteArray: get_string_byte_array,
}
