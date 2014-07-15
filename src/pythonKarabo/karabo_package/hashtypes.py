from __future__ import unicode_literals, absolute_import
import karabo.hash
from karabo.registry import Registry

import base64
import numpy


""" This module contains the type hierarchy implied by the Karabo hash.

This file closely corresponds to karabo.util.ReferenceType.

The C++ types are mostly implemented by using the corresponding numpy type."""


class Simple(object):
    @classmethod
    def read(cls, file):
        ret = numpy.frombuffer(file.data, cls.numpy, 1, file.pos)[0]
        file.pos += cls.numpy().itemsize
        return ret


    @classmethod
    def fromstring(cls, s):
        return cls.numpy(s)


    @classmethod
    def write(cls, file, data):
        file.writeFormat(cls.format, data)


    def cast(self, other):
        if isinstance(other, self.numpy):
            return other
        else:
            return self.numpy(other)


class Integer(Simple):
    def getMinMax(self):
        info = numpy.iinfo(self.numpy)
        min = self.minExc
        if min is None:
            min = self.minInc
        else:
            min += 1
        if min is None:
            min = info.min

        max = self.maxExc
        if max is None:
            max = self.maxInc
        else:
            max -= 1
        if max is None:
            max = info.max

        return min, max


class Number(Simple):
    def getMinMax(self):
        info = numpy.finfo(self.numpy)
        min = self.minExc
        if min is None:
            min = self.minInc
        else:
            min = min * (1 + numpy.sign(min) * info.eps) + info.tiny
        if min is None:
            min = info.min

        max = self.maxExc
        if max is None:
            max = self.maxInc
        else:
            max = max * (1 - numpy.sign(max) * info.eps) - info.tiny
        if max is None:
            max = info.max

        return min, max


class Vector(object):
    @classmethod
    def register(cls, name, dict):
        super(Vector, cls).register(name, dict)
        cls.__bases__[-1].vectortype = cls


    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        return (super(Vector, cls).read(file) for i in xrange(size))


    @classmethod
    def write(cls, file, data):
        file.writeFormat('I', len(data))
        for d in data:
            super(Vector, cls).write(file, d)


class NumpyVector(Vector):
    vstrs = { }


    @classmethod
    def register(cls, name, dict):
        super(NumpyVector, cls).register(name, dict)
        cls.vstrs[cls.numpy().dtype.str] = cls


    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        ret = numpy.frombuffer(file.data, cls.numpy, size, file.pos)
        file.pos += cls.numpy().itemsize * size
        return ret


    @classmethod
    def fromstring(cls, s):
        if s:
            return numpy.array([cls.numpy(x) for x in s.split(',')])
        else:
            return numpy.array([], dtype=cls.numpy)


    @classmethod
    def toString(cls, data):
        return ",".join(unicode(x) for x in data)


    def cast(self, other):
        if isinstance(other, numpy.ndarray) and other.dtype == self.numpy:
            ret = other
        else:
            ret = numpy.array(other, dtype=self.numpy)
        assert ret.ndim == 1, "can only treat one-dimensional vectors"
        return ret


class Descriptor(object):
    pass


class Special(object):
    """These is the base class for special types, which neither have
    a python nor numpy equivalent"""


class Type(Descriptor, Registry):
    """This is the base class for all types in the Karabo type hierarchy.

    The sub-classes of this class are an exact correspondance to the
    types defined in karabo.util.ReferenceType. The order of the
    sub-classes matters. Do not subclass this class unless the underlying
    C++ has been changed as well!"""
    types = [None] * 51
    fromname = { }
    strs = { }


    @classmethod
    def hashname(cls):
        s = ''
        lastlower = False
        for c in cls.__name__:
            if c.isupper() and lastlower:
                s += '_'
            s += c.capitalize()
            lastlower = c.islower()
        return s.rstrip('_')


    @classmethod
    def register(cls, name, dict):
        super(Type, cls).register(name, dict)
        cls.types[cls.number] = cls
        cls.fromname[cls.hashname()] = cls
        if 'numpy' in dict:
            cls.strs[cls.numpy().dtype.str] = cls


    @classmethod
    def toString(cls, data):
        return unicode(data)


    def toHash(self, box):
        return box.value


    def fromHash(self, box, data, timestamp=None):
        box._set(data, timestamp)


class Bool(Type):
    number = 0

    @classmethod
    def read(cls, file):
        return bool(Int8.read(file) != 0)


    @classmethod
    def write(cls, file, data):
        Int8.write(file, 1 if data else 0)


    @staticmethod
    def fromstring(s):
        return bool(int(s))


    @classmethod
    def toString(cls, data):
        return '1' if data else '0'


    def cast(self, other):
        return bool(other)


class VectorBool(NumpyVector, Bool):
    number = 1
    numpy = numpy.bool_


class Char(Simple, Type):
    number = 2
    numpy = numpy.uint8 # actually not used, for convenience only

    @staticmethod
    def read(file):
        file.pos += 1
        return Byte(file.data[file.pos - 1:file.pos])


    @classmethod
    def toString(cls, data):
        return base64.b64encode(data)


    @classmethod
    def fromstring(self, s):
        return base64.b64decode(s)


    @classmethod
    def write(cls, file, data):
        assert len(data) == 1
        file.file.write(data)


    def cast(self, other):
        if len(bytes(other)) == 1:
            return Byte(other)


class Byte(Special, bytes):
    """This represents just one byte, so that we can distinguish
    CHAR and VECTOR_CHAR."""
    hashtype = Char


    def __repr__(self):
        return "${:x}".format(ord(self))


class VectorChar(Vector, Char):
    """A VectorChar is simply some binary data in memory. The corresponding
    python data type is bytes. Make sure you don't use str for strings,
    as this will result in the hash creating a VectorChar, and the C++ will
    not be happy about this."""
    number = 3

    @staticmethod
    def read(file):
        size, = file.readFormat('I')
        file.pos += size
        return bytes(file.data[file.pos - size:file.pos])


    @classmethod
    def write(cls, file, data):
        file.writeFormat('I', len(data))
        file.file.write(data)


    def cast(self, other):
        if isinstance(other, bytes):
            return other
        else:
            return bytes(other)


class Int8(Integer, Type):
    number = 4
    format = "b"
    numpy = numpy.int8


class VectorInt8(NumpyVector, Int8):
    number = 5


class UInt8(Integer, Type):
    number = 6
    format = "B"
    numpy = numpy.uint8


class VectorUInt8(NumpyVector, UInt8):
    number = 7


class Int16(Integer, Type):
    number = 8
    format = "h"
    numpy = numpy.int16


class VectorInt16(NumpyVector, Int16):
    number = 9


class UInt16(Integer, Type):
    number = 10
    format = "H"
    numpy = numpy.uint16


class VectorUInt16(NumpyVector, UInt16):
    number = 11


class Int32(Integer, Type):
    number = 12
    format = "i"
    numpy = numpy.int32


class VectorInt32(NumpyVector, Int32):
    number = 13


class UInt32(Integer, Type):
    number = 14
    format = "I"
    numpy = numpy.uint32


class VectorUInt32(NumpyVector, UInt32):
    number = 15


class Int64(Integer, Type):
    number = 16
    format = "q"
    numpy = numpy.int64


class VectorInt64(NumpyVector, Int64):
    number = 17


class UInt64(Integer, Type):
    number = 18
    format = "Q"
    numpy = numpy.uint64


class VectorUInt64(NumpyVector, UInt64):
    number = 19


class Float(Number, Type):
    number = 20
    format = "f"
    numpy = numpy.float32


class VectorFloat(NumpyVector, Float):
    number = 21


class Double(Number, Type):
    number = 22
    format = "d"
    numpy = numpy.float64


class VectorDouble(NumpyVector, Double):
    number = 23


class ComplexFloat(Number, Type):
    number = 24
    format = "ff"
    numpy = numpy.complex64


class VectorComplexFloat(NumpyVector, ComplexFloat):
    number = 25


class ComplexDouble(Number, Type):
    number = 26
    format = "dd"
    numpy = numpy.complex128


class VectorComplexDouble(NumpyVector, ComplexDouble):
    number = 27


class String(Type):
    """This is the type corresponding to unicode strings, which are
    supposed to be used for all human-readable strings, so for
    everything except binary data."""
    number = 28

    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        file.pos += size
        return unicode(file.data[file.pos - size:file.pos], 'utf8')


    @staticmethod
    def fromstring(s):
        return unicode(s)


    @classmethod
    def write(cls, file, data):
        VectorChar.write(file, data.encode('utf8'))


    def cast(self, other):
        if isinstance(other, unicode):
            return other
        else:
            return unicode(other)


class VectorString(Vector, String):
    number = 29

    @staticmethod
    def fromstring(s):
        return StringList(s.split(','))


    @classmethod
    def read(cls, file):
        return StringList(super(VectorString, cls).read(file))


    @classmethod
    def toString(cls, data):
        return ",".join(unicode(x) for x in data)


    def cast(self, other):
        if isinstance(other, StringList):
            return other
        else:
            return StringList(unicode(s) for s in other)


class StringList(Special, list):
    """ This class represents a vector of strings """
    hashtype = VectorString


    def __repr__(self):
        return "$" + list.__repr__(self)


class Hash(Type):
    number = 30

    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        ret = karabo.hash.Hash()
        for i in xrange(size):
            key = file.readKey()
            type, = file.readFormat('I')
            type = cls.types[type]
            asize, = file.readFormat('I')
            attrs = { }
            for i in xrange(asize):
                akey = file.readKey()
                atype, = file.readFormat('I')
                atype = cls.types[atype]
                attrs[akey] = atype.read(file)
            ret[key] = type.read(file)
            ret[key, ...] = attrs
        return ret


    @classmethod
    def write(cls, file, data):
        file.writeFormat('I', len(data))
        for k, v in data.iteritems():
            file.writeKey(k)
            file.writeType(v)
            file.writeFormat('I', len(data[k, ...]))
            for ak, av in data[k, ...].iteritems():
                file.writeKey(ak)
                file.writeType(av)
                file.writeData(av)
            file.writeData(v)


    def cast(self, other):
        if isinstance(other, hash.Hash):
            return other
        else:
            raise TypeError(
                'cannot cast anything to Hash (was {})'.format(other))


class VectorHash(Vector, Hash):
    number = 31

    @classmethod
    def read(cls, file):
        return list(super(VectorHash, cls).read(file))


    def cast(self, other):
        return [Hash.cast(self, o) for o in other]


class Schema(Hash):
    number = 47

    @classmethod
    def read(cls, file):
        file.readFormat('I') # ignore length
        size, = file.readFormat('B')
        name = unicode(file.data[file.pos:file.pos + size], "utf8")
        file.pos += size
        ret = super(Schema, cls).read(file)
        return Schema_(name, ret)


class Schema_(Special):
    hashtype = Schema


    def __init__(self, name, hash):
        self.name = name
        self.hash = hash


class None_(Type):
    number = 50

    @staticmethod
    def read(file):
        file.readFormat('I') # ignore length
        return None


    @classmethod
    def toString(cls, data):
        return ''


    @classmethod
    def fromstring(self, s):
        return None


    @classmethod
    def write(cls, file, data):
        file.writeFormat('I', 0)


    def cast(self, other):
        if other is not None:
            raise TypeError('cannot cast to None (was {})'.format(other))
