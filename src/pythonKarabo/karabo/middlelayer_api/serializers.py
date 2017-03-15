from struct import calcsize, unpack
from xml.sax import make_parser
from xml.sax.saxutils import unescape
from xml.sax.handler import ContentHandler

from .hash import _gettype, Hash, HashList, HashType, Type


class BinaryParser(object):
    def readFormat(self, fmt):
        fmt = fmt.encode("ascii")
        size = calcsize(fmt)
        self.pos += size
        return unpack(fmt, self.data[self.pos - size:self.pos])

    def readKey(self):
        size, = self.readFormat('B')
        self.pos += size
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


def parseOne():
    name, attrs = yield "Start"
    hashattrs = {}
    for key, value in attrs.items():
        value = unescape(value)
        hashattrs[key] = value
        if value.startswith("KRB_") and ":" in value:
            dtype, svalue = value.split(":", 1)
            dtype = Type.fromname.get(dtype[4:], None)
            if dtype is not None:
                hashattrs[key] = dtype.fromstring(svalue)
    typename = hashattrs.pop("KRB_Type", "HASH")
    if typename == "HASH":
        value = yield from parseHash()
    elif typename == "VECTOR_HASH":
        value = yield from parseVectorHash()
    else:
        value = yield from parseString(typename)
    return name, value, hashattrs


def parseVectorHash():
    ret = HashList()
    try:
        while True:
            name, _ = yield "Start"
            assert name == "KRB_Item"
            ret.append((yield from parseHash()))
    except EndElement:
        return ret


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
