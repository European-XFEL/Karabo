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
import base64
from pathlib import Path
from xml.sax.saxutils import escape, quoteattr

from .hash import Hash, get_hash_type_from_data
from .typenums import HASH_TYPE_TO_XML_TYPE, HashType

__all__ = ['encodeXML', 'writeXML', 'saveToFile']


def yieldXML(data):
    if len(data) == 1 and isinstance(next(iter(data.values())), Hash):
        yield from yield_xml_hash(data)
    else:
        yield '<root KRB_Artificial="">'
        yield from yield_xml_hash(data)
        yield '</root>'


def encodeXML(data):
    return "".join(yieldXML(data))


def writeXML(data, file):
    for d in yieldXML(data):
        file.write(d)


def yield_xml_hash(data):
    for key, value, attrs in data.iterall():
        value_type = get_hash_type_from_data(value)
        value_name = HASH_TYPE_TO_XML_TYPE[value_type]
        value_writer = __WRITER_MAP[value_type]
        yield '<{key} KRB_Type="{value_type}" '.format(
            key=key, value_type=value_name)
        for k, v in attrs.items():
            attr_type = get_hash_type_from_data(v)
            attr_name = HASH_TYPE_TO_XML_TYPE[attr_type]
            attr_writer = __WRITER_MAP[attr_type]
            attr = "KRB_{attr_type}:{data}".format(
                attr_type=attr_name,
                data="".join(attr_writer(v)))
            yield '{key}={attr} '.format(
                key=k, attr=quoteattr(attr))
        yield ">"

        yield from value_writer(value)
        yield f"</{key}>"


def yield_xml_simple(data):
    yield escape(str(data))


def yield_xml_numpy_vector(data):
    yield escape(",".join(str(x) for x in data))


def yield_xml_vector_char(data):
    yield escape(base64.b64encode(data).decode("ascii"))


def yield_xml_byte_array(data):
    yield escape(base64.b64encode(data).decode("ascii"))


def yield_xml_vector_bool(data):
    yield escape(",".join(str(int(i)) for i in data))


def yield_xml_list(data):
    yield escape(",".join(str(x) for x in data))


def yield_xml_vector_hash(data):
    for d in data:
        yield "<KRB_Item>"
        yield from yield_xml_hash(d)
        yield "</KRB_Item>"


def yield_xml_schema(data):
    yield data.name
    yield ":"
    yield escape("".join(yield_xml_hash(data.hash)))


def yield_xml_bool(data):
    yield escape('1' if data else '0')


__WRITER_MAP = {
    HashType.Bool: yield_xml_bool,
    HashType.Char: yield_xml_simple,
    HashType.Int8: yield_xml_simple,
    HashType.Int16: yield_xml_simple,
    HashType.Int32: yield_xml_simple,
    HashType.Int64: yield_xml_simple,
    HashType.UInt8: yield_xml_simple,
    HashType.UInt16: yield_xml_simple,
    HashType.UInt32: yield_xml_simple,
    HashType.UInt64: yield_xml_simple,
    HashType.Float: yield_xml_simple,
    HashType.Double: yield_xml_simple,

    HashType.VectorBool: yield_xml_vector_bool,
    HashType.VectorChar: yield_xml_vector_char,
    HashType.VectorInt8: yield_xml_numpy_vector,
    HashType.VectorInt16: yield_xml_numpy_vector,
    HashType.VectorInt32: yield_xml_numpy_vector,
    HashType.VectorInt64: yield_xml_numpy_vector,
    HashType.VectorUInt8: yield_xml_numpy_vector,
    HashType.VectorUInt16: yield_xml_numpy_vector,
    HashType.VectorUInt32: yield_xml_numpy_vector,
    HashType.VectorUInt64: yield_xml_numpy_vector,
    HashType.VectorFloat: yield_xml_numpy_vector,
    HashType.VectorDouble: yield_xml_numpy_vector,

    HashType.String: yield_xml_simple,
    HashType.VectorString: yield_xml_list,
    HashType.Hash: yield_xml_hash,
    HashType.VectorHash: yield_xml_vector_hash,
    HashType.Schema: yield_xml_schema,
    HashType.None_: yield_xml_simple,
    HashType.ByteArray: yield_xml_byte_array,
}


def saveToFile(hash_, filename):
    """Write a Hash to XML

    Note: If the file already exists, it gets overwritten!
    """

    assert isinstance(hash_, Hash), f"Expected Hash, not {type(hash_)}"
    filename = Path(filename)
    directory = filename.parent.absolute()
    directory.mkdir(parents=True, exist_ok=True)

    with filename.open('w') as fout:
        fout.write(encodeXML(hash_))
