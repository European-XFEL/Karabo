import os
import os.path as op
import re
from struct import calcsize, unpack
import sys
import warnings
from xml.sax import make_parser, SAXException
from xml.sax.saxutils import unescape
from xml.sax.handler import ContentHandler

from karabo.native.data.hash import (
    _gettype, Hash, HashList, HashType, SchemaHashType, Type, VectorHash)
from karabo.native.data.serializers import (
    EndElement, parseString, XMLParser)


SEC_TO_USEC = 1000000
USEC_TO_ATTOSEC = 1000000000000


def isValidRawFileName(path):
    """
    Return True if full path is a sensible Karabo DataLogger path
    """
    if not op.isfile(path):
        return False

    name_pattern = re.compile(r"archive_\d+\.txt")
    if not name_pattern.match(op.basename(path)):
        return False

    return True


def device_id_from_path(log_data_path):
    """Extracts the device_id from a given path.

    :param log_data_path: the path which is assumed to be of the form
    '*/karaboHistory/[device_id]/raw' - this is the standard path for
    file-based log files in Karabo.
    :return: the id of the device as extracted from the path.
    :raises RuntimeError: if the path is not in the assumed format.
    """
    dev_id = ""
    if isValidRawFileName(log_data_path):
        dirname = op.dirname(log_data_path)
        # erase dirname from begin to 'karaboHistory/' included
        dirname = dirname[dirname.find('karaboHistory/') +
                          len('karaboHistory/'):]
        dev_id = dirname.rstrip('/raw')
    else:
        raise RuntimeError("invalid raw file path: "
                           "{}".format(log_data_path))
    return dev_id


def format_line_protocol_body(data):
    """Formats a list of strings (or a single string) to conform to
    InfluxDb's line protocol body requirements.

    :param data:either a string or a list of strings to be formatted
    :return:data formatted as a single string conformant to the line
    protocol and ready to be placed in the body of a write request to
    influxdb.
    """
    if isinstance(data, str):
        data = [data]
    data = ('\n'.join(data) + '\n').encode('utf-8')
    return data


def escape_tag_field_key(s):
    """Escapes a string to be used as a tag key, tag value or field key.

    Escaping requirements extracted from
    https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference/
    """
    s = s.replace("\\", "\\\\")
    s = s.replace(",", "\\,")
    s = s.replace("=", "\\=")
    s = s.replace(" ", "\\ ")
    return s


def escape_measurement(s):
    """Escapes a string to be used as a measurement name.

    Escaping requirements extracted from
    https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference/
    """
    s = s.replace(",", "\\,")
    s = s.replace(" ", "\\ ")
    return s


  # This portion of the code is temporary and serves only to parse the
  # XML files created by the bound library. They are mainly a altered version
  # of the ones in the karabo.native module.
  # Since this is used mainly for migration purposes this inferior practice
  # is tolerated


def parsePendingAttrs(to_fill, hashAttrs):
    while len(to_fill) > 0:
        name, value, attrs = yield from parseOne()
        if name in to_fill:
            hashAttrs[name.split('_')[-1]] = value[name+'_value']
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
            dtype = Type.fromname.get(dtype[4:], None)
            if dtype in (SchemaHashType, VectorHash):
                hashattrs.pop(key)
                to_fill.add(svalue)
            elif dtype is not None:
                hashattrs[key] = dtype.fromstring(svalue)
    # Exhaust the child nodes while there are still attribute values to be filled.
    # Each attribute that has not been filled should be matched by a child with its
    # name.
    yield from parsePendingAttrs(to_fill, hashattrs)

    typename = hashattrs.pop("KRB_Type", "HASH")
    if typename == "HASH":
        value = yield from parseHash()
    elif typename == "VECTOR_HASH":
        attrs, value = yield from parseVectorHash(to_fill)
    else:
        value = yield from parseString(typename)

    return name, value, hashattrs

def parseHash():
    ret = Hash()
    try:
        while True:
            name, value, attrs = yield from parseOne()
            ret[name] = value
            ret[name, ...] = attrs
    except EndElement:
        return ret

def parseVectorHash(schema_attrs):
    ret = HashList()
    attrs = {}
    try:
        while True:
            name, _ = yield "Start"
            if name in schema_attrs:
                attrs[name] = (yield from parseHash())
            elif name == "KRB_Item":
                ret.append((yield from parseHash()))
    except EndElement:
        return attrs, ret


class SchemaXMLParser(XMLParser):
    def parseAll(self):
        self.name, self.value, self.hashattrs = yield from parseOne()


def decodeSchemaXML(data):
    parser = make_parser()
    handler = SchemaXMLParser()
    parser.setContentHandler(handler)
    parser.feed(data)
    parser.close()
    if handler.name == "root" and "KRB_Artificial" in handler.hashattrs:
        return handler.value
    else:
        ret = Hash(handler.name, handler.value)
        ret[handler.name, ...] = handler.hashattrs
        return ret
