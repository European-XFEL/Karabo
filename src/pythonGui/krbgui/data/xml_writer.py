from base64 import b64encode
from io import StringIO
from xml.etree.ElementTree import ElementTree, Element

from .typenums import (
    HashType, HASH_TYPE_TO_XML_TYPE, HASH_TYPE_TO_XML_ATTR_TYPE
)


def write_xml_hash(hsh):
    fp = StringIO()
    with fp:
        attrs = {'KRB_Type': HASH_TYPE_TO_XML_TYPE[HashType.Hash],
                 'KRB_Artificial': ''}
        e = Element("root", attrib=attrs)

        for el in _walk_hash(hsh):
            e.append(el)
        et = ElementTree(e)
        et.write(fp, encoding='unicode')
        return fp.getvalue()


def write_xml_schema(schema):
    fp = StringIO()
    with fp:
        fp.write('{}:'.format(schema.name))
        fp.write(write_xml_hash(schema.hash))
        return fp.getvalue()


def _convert_attributes(attrs):
    ret = {'KRB_Type': HASH_TYPE_TO_XML_TYPE[attrs['KRB_Type']]}
    attr_types = attrs['KRB_AttrTypes']
    for k, v in attrs.items():
        if k not in attr_types:
            continue
        atype_enum = attr_types[k]
        atype_name = HASH_TYPE_TO_XML_ATTR_TYPE[atype_enum]
        convert = _CONVERTER_MAP[atype_enum]
        ret[k] = '{}:{}'.format(atype_name, convert(v))

    return ret


def _walk_hash(hsh):
    for k, v, a in hsh.iterall():
        type_enum = a['KRB_Type']
        elem = Element(k, attrib=_convert_attributes(a))
        if type_enum in _CONVERTER_MAP:
            convert = _CONVERTER_MAP[type_enum]
            elem.text = convert(v)
        elif type_enum is HashType.Hash:
            for el in _walk_hash(v):
                elem.append(el)
        elif type_enum is HashType.HashList:
            for sv in v:
                sel = Element('KRB_Item')
                for el in _walk_hash(sv):
                    sel.append(el)
                elem.append(sel)
        yield elem


def _complex_convert(value):
    return '({},{})'.format(value.real, value.imag)


def _vector_convert(value):
    if len(value) == 0:
        return ''
    return ','.join(str(v) for v in value)

_CONVERTER_MAP = {
    HashType.Bool: lambda x: '1' if x else '0',
    HashType.Char: str,
    HashType.Int8: str,
    HashType.Int16: str,
    HashType.Int32: str,
    HashType.Int64: str,
    HashType.UInt8: str,
    HashType.UInt16: str,
    HashType.UInt32: str,
    HashType.UInt64: str,
    HashType.Float32: str,
    HashType.Float64: str,
    HashType.Complex64: _complex_convert,
    HashType.Complex128: _complex_convert,

    HashType.BoolArray: _vector_convert,
    HashType.Bytes: lambda x: b64encode(x).decode("ascii"),
    HashType.Int8Array: _vector_convert,
    HashType.Int16Array: _vector_convert,
    HashType.Int32Array: _vector_convert,
    HashType.Int64Array: _vector_convert,
    HashType.UInt8Array: _vector_convert,
    HashType.UInt16Array: _vector_convert,
    HashType.UInt32Array: _vector_convert,
    HashType.UInt64Array: _vector_convert,
    HashType.Float32Array: _vector_convert,
    HashType.Float64Array: _vector_convert,
    HashType.Complex64Array: _vector_convert,
    HashType.Complex128Array: _vector_convert,

    # HashType.Hash and HashType.HashList are handled by _walk_hash()
    HashType.Schema: write_xml_schema,
    HashType.None_: str,
    HashType.String: str,
    HashType.StringList: _vector_convert,
}
