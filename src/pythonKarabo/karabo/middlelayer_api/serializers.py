from struct import calcsize, unpack

from .hash import _gettype, HashType


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
