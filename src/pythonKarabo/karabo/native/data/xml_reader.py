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
from functools import partial
from pathlib import Path
from xml.sax import SAXException, make_parser
from xml.sax.handler import ContentHandler
from xml.sax.saxutils import unescape

import numpy as np

from .hash import Hash, HashList
from .schema import Schema
from .typenums import XML_TYPE_TO_HASH_TYPE, HashType

__all__ = ['decodeXML', 'loadFromFile', 'XMLParser']


# XML is parsed using a SAX parser. This SAX parser sends all the parsed
# elements into a generator, which are the following functions whose names
# start with "parse". They yield "Start" if they expect the start
# of an element, or "Chars" if they expect a string to be read.

def parseHash():
    ret = Hash()
    try:
        while True:
            name, value, attrs = yield from parseOne()
            ret.setElement(name, value, attrs)
    except EndElement:
        return ret


def readPendingAttrs(to_fill, hashAttrs):
    while len(to_fill) > 0:
        name, value, attrs = yield from parseOne()
        if name in to_fill:
            hashAttrs[name.split('_')[-1]] = value[name + '_value']
            to_fill.remove(name)


def parseOne():
    name, attrs = yield "Start"
    hashattrs = {}
    to_fill = set()
    for key, value in attrs.items():
        value = unescape(value)
        hashattrs[key] = value
        if value.startswith("KRB_") and ":" in value:
            dtype, svalue = value.split(":", 1)
            dtype = XML_TYPE_TO_HASH_TYPE.get(dtype[4:], None)
            if dtype in (HashType.Schema, HashType.VectorHash):
                hashattrs.pop(key)
                if svalue.startswith('_attr_root_'):
                    # we are dealing with a VectorHash or Schema attribute
                    # encoded by Karabo 2.6.0 or later; it originated a child
                    # node which has to be parsed. Attributes of those types
                    # prior to Karabo 2.6.0 should be handled as simple strings
                    to_fill.add(svalue)
                else:
                    hashattrs[key] = svalue
            elif dtype is not None:
                reader = __READER_MAP[dtype]
                hashattrs[key] = reader(svalue)
    # Process any existing child node that corresponds to an attribute of type
    # Schema or VectorHash as encoded by Karabo 2.6.0 or later.
    yield from readPendingAttrs(to_fill, hashattrs)
    typename = hashattrs.pop("KRB_Type", "HASH")
    if typename == "HASH":
        value = yield from parseHash()
    elif typename == "VECTOR_HASH":
        value, attrs = yield from parseVectorHash(to_fill)
    else:
        value = yield from parseString(typename)
    return name, value, hashattrs


def parseVectorHash(schema_attrs):
    ret = HashList()
    attrs = {}
    try:
        while True:
            name, _ = yield "Start"
            if schema_attrs and name in schema_attrs:
                attrs[name] = (yield from parseHash())
            else:
                assert name == "KRB_Item"
                ret.append((yield from parseHash()))
    except EndElement:
        return ret, attrs


def parseString(typename):
    ret = []
    try:
        while True:
            ret.append((yield "Chars"))
    except EndElement:
        value_type = XML_TYPE_TO_HASH_TYPE[typename]
        reader = __READER_MAP[value_type]
        return reader("".join(ret))


class EndElement(Exception):
    pass


class XMLParser(ContentHandler):
    """Convert a valid Karabo XML string into its Karabo representation

    Converting a given string back to a hash, for instance, is done so:

        >>> s = ('<root KRB_Artificial=""><a KRB_Type="INT32" >1</a>'
                 '<b KRB_Type="INT32" >2</b></root>"')
        >>> h = XMLParser.read(s)
        >>> isinstance(h, Hash)
        True

    Reading from a file can be done as follows:

        >>> with open("file.xml") as fin:
                h = XMLParser.read(fin.read())
        >>> isinstance(h, Hash)
        True
    """

    def __init__(self):
        super().__init__()
        self.parser = self.parseAll()
        self.last = self.parser.send(None)

    def parseAll(self):
        self.name, self.value, self.hashattrs = yield from parseOne()

    def startElement(self, name, attrs):
        assert self.last == "Start"
        self.last = self.parser.send((name, attrs))

    def characters(self, content):
        if self.last == "Chars":
            self.parser.send(content)

    def endElement(self, name):
        try:
            self.last = self.parser.throw(EndElement())
        except StopIteration:
            pass


def decodeXML(data):
    parser = make_parser()
    handler = XMLParser()
    parser.setContentHandler(handler)
    parser.feed(data)
    parser.close()
    if handler.name == "root" and "KRB_Artificial" in handler.hashattrs:
        return handler.value
    else:
        ret = Hash(handler.name, handler.value)
        ret[handler.name, ...] = handler.hashattrs
        return ret


def read_xml_complex(data):
    return complex(*[float(n) for n in data[1:-1].split(',')])


def read_xml_bool(data):
    return bool(int(data))


def read_xml_char(data):
    # This is the status of the MDL. Maybe check to return different
    return data


def read_xml_string(data):
    return data


def read_xml_numpy(data, numpy=None):
    return numpy(data)


def read_xml_numpy_vector(data, numpy=None):
    if data:
        return np.fromstring(data, sep=",", dtype=numpy)
    else:
        return np.array([], dtype=numpy)


def read_xml_schema(data):
    name, xml = data.split(":", 1)
    return Schema(name, hash=decodeXML(xml))


def read_xml_empty(data):
    return None


def read_xml_vector_string(data):
    if not data:
        return []
    return [ss.strip() for ss in data.split(',')]


def read_xml_bytearray(data):
    # XXX: should return bytearray(base64.b64decode(s))
    return base64.b64decode(data)


def read_xml_vector_char(data):
    return base64.b64decode(data)


# HashType.Hash and HashType.VectorHash are not handled in the reader map
__READER_MAP = {
    HashType.Bool: read_xml_bool,
    HashType.Char: read_xml_char,
    HashType.Int8: partial(read_xml_numpy, numpy=np.int8),
    HashType.UInt8: partial(read_xml_numpy, numpy=np.uint8),
    HashType.Int16: partial(read_xml_numpy, numpy=np.int16),
    HashType.UInt16: partial(read_xml_numpy, numpy=np.uint16),
    HashType.Int32: partial(read_xml_numpy, numpy=np.int32),
    HashType.UInt32: partial(read_xml_numpy, numpy=np.uint32),
    HashType.Int64: partial(read_xml_numpy, numpy=np.int64),
    HashType.UInt64: partial(read_xml_numpy, numpy=np.uint64),
    HashType.Float: partial(read_xml_numpy, numpy=np.float32),
    HashType.Double: partial(read_xml_numpy, numpy=np.float64),
    HashType.ComplexFloat: read_xml_complex,
    HashType.ComplexDouble: read_xml_complex,

    HashType.VectorBool: partial(read_xml_numpy_vector, numpy=np.bool_),
    HashType.VectorChar: read_xml_vector_char,
    HashType.VectorInt8: partial(read_xml_numpy_vector, numpy=np.int8),
    HashType.VectorUInt8: partial(read_xml_numpy_vector, numpy=np.uint8),
    HashType.VectorInt16: partial(read_xml_numpy_vector, numpy=np.int16),
    HashType.VectorUInt16: partial(read_xml_numpy_vector, numpy=np.uint16),
    HashType.VectorInt32: partial(read_xml_numpy_vector, numpy=np.int32),
    HashType.VectorUInt32: partial(read_xml_numpy_vector, numpy=np.uint32),
    HashType.VectorInt64: partial(read_xml_numpy_vector, numpy=np.int64),
    HashType.VectorUInt64: partial(read_xml_numpy_vector, numpy=np.uint64),
    HashType.VectorFloat: partial(read_xml_numpy_vector, numpy=np.float32),
    HashType.VectorDouble: partial(read_xml_numpy_vector, numpy=np.float64),
    HashType.VectorComplexFloat: partial(read_xml_numpy_vector,
                                         numpy=np.complex64),
    HashType.VectorComplexDouble: partial(read_xml_numpy_vector,
                                          numpy=np.complex128),

    HashType.String: read_xml_string,
    HashType.VectorString: read_xml_vector_string,
    HashType.Schema: read_xml_schema,
    HashType.None_: read_xml_empty,
    HashType.ByteArray: read_xml_bytearray,
}


def loadFromFile(filename):
    """Load a Hash from file.

    If the file does not exist, then return None
    """
    filename = Path(filename)
    if not filename.exists():
        return

    with filename.open('r') as fin:
        try:
            hash_ = decodeXML(fin.read())
            return hash_
        except SAXException:
            raise RuntimeError(f"{filename} is malformatted.")
