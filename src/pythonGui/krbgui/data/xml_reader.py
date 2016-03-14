from base64 import b64decode
from xml.etree.ElementTree import XMLParser, TreeBuilder

import numpy as np
from .hash import Hash, Schema
from .typenums import (
    HashType, XML_TYPE_TO_HASH_TYPE, XML_ATTR_TYPE_TO_HASH_TYPE
)


def read_xml_hash(data):
    tb = TreeBuilder(element_factory=ElementBuilder())
    p = XMLParser(target=tb)
    p.feed(data)
    root = tb.close()
    if root.artificial:
        ret = root.data
    else:
        ret = Hash(root.tag, root.data)
        ret[root.tag, ...] = root.attrs

    return ret


def read_xml_schema(data):
    name, data = data.split(':', 1)
    h = read_xml_hash(data)
    return Schema(name=name, hash=h)


def _array_convert(dtype):
    def converter(value):
        if len(value) == 0:
            return np.array([], dtype=dtype)
        return np.fromstring(value, dtype=dtype, sep=',')
    return converter


def _complex_convert(value):
    parts = value[1:-1].split(',')
    assert len(parts) == 2
    return complex(*[float(n) for n in parts])


def _string_array_convert(value):
    if len(value) == 0:
        return []
    return value.split(',')


def _passthrough(x):
    return x

_CONVERTER_MAP = {
    HashType.Bool: lambda x: True if x == '1' else False,
    HashType.Char: int,
    HashType.Int8: int,
    HashType.Int16: int,
    HashType.Int32: int,
    HashType.Int64: int,
    HashType.UInt8: int,
    HashType.UInt16: int,
    HashType.UInt32: int,
    HashType.UInt64: int,
    HashType.Float32: float,
    HashType.Float64: float,
    HashType.Complex64: _complex_convert,
    HashType.Complex128: _complex_convert,

    HashType.BoolArray: _array_convert(np.bool_),
    HashType.Bytes: b64decode,
    HashType.Int8Array: _array_convert(np.int8),
    HashType.Int16Array: _array_convert(np.int16),
    HashType.Int32Array: _array_convert(np.int32),
    HashType.Int64Array: _array_convert(np.int64),
    HashType.UInt8Array: _array_convert(np.uint8),
    HashType.UInt16Array: _array_convert(np.uint16),
    HashType.UInt32Array: _array_convert(np.uint32),
    HashType.UInt64Array: _array_convert(np.uint64),
    HashType.Float32Array: _array_convert(np.float32),
    HashType.Float64Array: _array_convert(np.float64),
    HashType.Complex64Array: _array_convert(np.complex64),
    HashType.Complex128Array: _array_convert(np.complex128),

    HashType.Hash: _passthrough,
    HashType.Schema: read_xml_schema,
    HashType.None_: lambda _: None,
    HashType.String: _passthrough,
    HashType.StringList: _string_array_convert,
}


class XMLElement(object):
    def __init__(self, tag, attrs=None):
        self.tag = tag
        self.artificial = ('KRB_Artificial' in attrs)
        type_enum = XML_TYPE_TO_HASH_TYPE[attrs.get('KRB_Type', 'NONE')]
        attr_types = {}
        self.attrs = {'KRB_Type': type_enum, 'KRB_AttrTypes': attr_types}
        for k, v in attrs.items():
            if k.startswith('KRB_'):
                continue
            atype_enum, aval = self._parse_attr_value(v)
            attr_types[k] = atype_enum
            self.attrs[k] = aval

    def append(self, elem):
        raise RuntimeError("You can't append to XMLElement")

    @property
    def data(self):
        return getattr(self, 'text', '')

    @staticmethod
    def _parse_attr_value(value):
        key, val = value.split(":", 1)
        type_enum = XML_ATTR_TYPE_TO_HASH_TYPE[key]
        converter = _CONVERTER_MAP.get(type_enum, _passthrough)
        return type_enum, converter(val)


class ListElement(XMLElement):
    def __init__(self, tag, attrs=None):
        super(ListElement, self).__init__(tag, attrs)
        self.children = []

    def append(self, elem):
        self.children.append(elem)

    @property
    def data(self):
        return [e.data for e in self.children]


class HashElement(XMLElement):
    def __init__(self, tag, attrs=None):
        super(HashElement, self).__init__(tag, attrs)
        self.children = []

    def append(self, elem):
        self.children.append(elem)

    @property
    def data(self):
        h = Hash()
        for elem in self.children:
            attributes = elem.attrs
            type_enum = attributes['KRB_Type']
            converter = _CONVERTER_MAP.get(type_enum, _passthrough)
            h[elem.tag] = converter(elem.data)
            h[elem.tag, ...] = attributes
        return h


class ElementBuilder(object):
    _last = None

    def __call__(self, tag, attrs):
        krb_type = attrs.get("KRB_Type", "")
        if krb_type == "HASH":
            return HashElement(tag, attrs)
        elif krb_type == "VECTOR_HASH":
            return ListElement(tag, attrs)
        elif tag == "KRB_Item":
            return HashElement("", attrs)
        else:
            return XMLElement(tag, attrs)
