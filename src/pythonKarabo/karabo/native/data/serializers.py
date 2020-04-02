import os
from struct import calcsize, unpack
import warnings
from xml.sax import make_parser, SAXException
from xml.sax.saxutils import unescape
from xml.sax.handler import ContentHandler

from karabo.native.data.hash import (
    _gettype, Hash, HashList, HashType, SchemaHashType, Type, VectorHash)


class BinaryParser(object):
    def readFormat(self, fmt):
        fmt = fmt.encode("ascii")
        size = calcsize(fmt)
        self.pos += size
        return unpack(fmt, self.data[self.pos - size:self.pos])

    def readKey(self):
        size, = unpack('B', self.data[self.pos:self.pos + 1])
        self.pos += size + 1
        return self.data[self.pos - size:self.pos].decode("ascii")

    def read(self, data):
        self.pos = 0
        self.data = data
        return HashType.read(self)


def decodeBinary(data):
    parser = BinaryParser()
    return parser.read(data)


def encodeBinary(data):
    """return the binary serialization of hash `data`"""
    hashtype = _gettype(data)
    return b"".join(hashtype.yieldBinary(data))


def writeBinary(data, file):
    """write the hash in `data` into `file`"""
    hashtype = _gettype(data)
    for d in hashtype.yieldBinary(data):
        file.write(d)


# XML is parsed using a SAX parser. This SAX parser sends all the parsed
# elements into a generator, which are the following functions whose names
# start with "parse". They yield "Start" if they expect the start
# of an element, or "Chars" if they expect a string to be read.

def parseHash():
    ret = Hash()
    try:
        while True:
            name, value, attrs = yield from parseOne()
            ret[name] = value
            ret[name, ...] = attrs
    except EndElement:
        return ret


def readPendingAttrs(to_fill, hashAttrs):
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
                if svalue.startswith('_attr_root_'):
                    # we are dealing with a VectorHash or Schema attribute
                    # encoded by Karabo 2.6.0 or later; it originated a child
                    # node which has to be parsed. Attributes of those types
                    # prior to Karabo 2.6.0 should be handled as simple strings
                    to_fill.add(svalue)
                else:
                    hashattrs[key] = svalue
            elif dtype is not None:
                hashattrs[key] = dtype.fromstring(svalue)
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
        return Type.fromname[typename].fromstring("".join(ret))


class EndElement(Exception):
    pass


class XMLParser(ContentHandler):
    """Convert a valid Karabo XML string into its middlelayer representation

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

    # legacy API, has nothing to do with rest of class
    @staticmethod
    def read(data):
        warnings.warn(
            "The XMLParser class is deprecated, use decodeXML instead",
            DeprecationWarning)
        return decodeXML(data)


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


def yieldXML(data):
    if len(data) == 1 and isinstance(next(iter(data.values())), Hash):
        yield from HashType.yieldXML(data)
    else:
        yield '<root KRB_Artificial="">'
        yield from HashType.yieldXML(data)
        yield '</root>'


def encodeXML(data):
    return "".join(yieldXML(data))


def writeXML(data, file):
    for d in yieldXML(data):
        file.write(d)


def saveToFile(hash_, filename):
    """Write a Hash to XML

    If the file already exists, it gets overwritten
    """

    assert isinstance(hash_, Hash), "Expected Hash, not {}".format(type(hash_))
    directory = os.path.dirname(filename)
    if directory:
        os.makedirs(directory, exist_ok=True)

    with open(filename, "w") as fout:
        fout.write(encodeXML(hash_))


def loadFromFile(filename):
    """Load a Hash from file.

    If the file does not exist, then return None
    """
    if not os.path.exists(filename):
        return

    with open(filename) as fin:
        try:
            hash_ = decodeXML(fin.read())
            return hash_
        except SAXException:
            raise "{} malformatted".format(filename)


# legacy API
class XMLWriter(object):
    @staticmethod
    def write(data):
        warnings.warn(
            "The XMLWriter class is deprecated, use encodeXML instead",
            DeprecationWarning)
        return encodeXML(data)
