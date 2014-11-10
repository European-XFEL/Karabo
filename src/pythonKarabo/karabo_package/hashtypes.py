
import karabo.hash
from karabo.registry import Registry
from karabo.enums import AccessLevel, AccessMode, Assignment

from asyncio import coroutine
import base64
from enum import Enum
import numpy


""" This module contains the type hierarchy implied by the Karabo hash.

This file closely corresponds to karabo.util.ReferenceType.

The C++ types are mostly implemented by using the corresponding numpy type."""


class Attribute(object):
    def __init__(self, default=None):
        self.default = default


    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            return instance.__dict__.get(self, self.default)


    def __set__(self, instance, value):
        instance.__dict__[self] = value


class Enumable(object):
    def __init__(self, enum=None, **kwargs):
        super().__init__(**kwargs)
        assert enum is None or issubclass(enum, Enum)
        self.enum = enum


    def cast(self, other):
        if isinstance(other, self.enum):
            return other
        else:
            raise TypeError("{} required here".format(self.enum))


    def asHash(self, data):
        if self.enum is None:
            return self.cast(data)
        else:
            return data.value


class Simple(object):
    minExc = Attribute()
    maxExc = Attribute()
    minInc = Attribute()
    maxInc = Attribute()

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
        if self.enum is not None:
            return super().cast(other)
        elif isinstance(other, self.numpy):
            return other
        else:
            return self.numpy(other)


class Integer(Simple, Enumable):
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
        return (super(Vector, cls).read(file) for i in range(size))


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
        return ",".join(str(x) for x in data)


    def cast(self, other):
        if isinstance(other, numpy.ndarray) and other.dtype == self.numpy:
            ret = other
        else:
            ret = numpy.array(other, dtype=self.numpy)
        assert ret.ndim == 1, "can only treat one-dimensional vectors"
        return ret


class Descriptor(object):
    displayedName = Attribute()
    alias = Attribute()
    description = Attribute()
    allowedStates = Attribute()
    defaultValue = Attribute()
    accessMode = Attribute(AccessMode.RECONFIGURABLE)
    assignment = Attribute(Assignment.OPTIONAL)
    requiredAccessLevel = Attribute(AccessLevel.OBSERVER)
    displayType = Attribute()
    enum = None

    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            if isinstance(getattr(self.__class__, k, None), Attribute):
                setattr(self, k, v)
            else:
                raise TypeError("__init__ got unexpected keyword argument: {}".
                                format(k))


    def parameters(self):
        return {p: getattr(self, p) for p in dir(type(self))
                if isinstance(getattr(type(self), p), Attribute) and
                   getattr(self, p) is not None}


    def subschema(self):
        return karabo.hash.Hash()


    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            if self not in instance.__dict__:
                raise AttributeError
            return instance.__dict__[self]


    def __set__(self, instance, value):
        v = self.cast(value)
        instance.setValue(self, v)


class Slot(Descriptor):
    def parameters(self):
        ret = super(Slot, self).parameters()
        ret["nodeType"] = 1
        ret["displayType"] = "Slot"
        return ret


    def asHash(self, other):
        return karabo.hash.Hash()


    def cast(self, other):
        return karabo.hash.Hash()


    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            return self.method.__get__(instance, owner)


    def __call__(self, method):
        self.method = coroutine(method)
        return self


class Special(object):
    """These is the base class for special types, which neither have
    a python nor numpy equivalent"""


class Type(Descriptor, Registry):
    """This is the base class for all types in the Karabo type hierarchy. """
    types = [None] * 51
    fromname = { }
    strs = { }

    options = Attribute()
    unitSymbol = Attribute("")
    metricPrefixSymbol = Attribute("")


    @classmethod
    def hashname(cls):
        return cls._hashname

    @classmethod
    def register(cls, name, dict):
        super(Type, cls).register(name, dict)
        cls.types[cls.number] = cls

        if "number" in dict:
            s = ''
            lastlower = False
            for c in cls.__name__:
                if c.isupper() and lastlower:
                    s += '_'
                s += c.capitalize()
                lastlower = c.islower()
            cls._hashname = s.rstrip('_')

        cls.fromname[cls._hashname] = cls
        if 'numpy' in dict:
            cls.strs[cls.numpy().dtype.str] = cls

    @classmethod
    def toString(cls, data):
        return str(data)


    def asHash(self, data):
        return self.cast(data)


    def parameters(self):
        ret = super(Type, self).parameters()
        ret["nodeType"] = 0
        ret["valueType"] = self.hashname()
        return ret


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
        return base64.b64encode(data).decode("ascii")


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


class String(Enumable, Type):
    """This is the type corresponding to unicode strings, which are
    supposed to be used for all human-readable strings, so for
    everything except binary data."""
    number = 28

    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        file.pos += size
        return str(file.data[file.pos - size:file.pos], 'utf8')


    @staticmethod
    def fromstring(s):
        return str(s)


    @classmethod
    def write(cls, file, data):
        VectorChar.write(file, data.encode('utf8'))


    def cast(self, other):
        if self.enum is not None:
            return super().cast(other)
        elif isinstance(other, str):
            return other
        else:
            return str(other)


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
        return ",".join(str(x) for x in data)


    def cast(self, other):
        if isinstance(other, StringList):
            return other
        else:
            return StringList(str(s) for s in other)


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
        for i in range(size):
            key = file.readKey()
            type, = file.readFormat('I')
            type = cls.types[type]
            asize, = file.readFormat('I')
            attrs = { }
            for i in range(asize):
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
        for k, v in data.items():
            file.writeKey(k)
            file.writeType(v)
            file.writeFormat('I', len(data[k, ...]))
            for ak, av in data[k, ...].items():
                file.writeKey(ak)
                file.writeType(av)
                file.writeData(av)
            file.writeData(v)


    def cast(self, other):
        if other.hashtype is type(self):
            return other
        else:
            raise TypeError('cannot cast anything to {} (was {})'.
                            format(type(self).__name__, type(other).__name__))


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
        l, = file.readFormat('I') # ignore length
        op = file.pos
        size, = file.readFormat('B')
        name = str(file.data[file.pos:file.pos + size], "utf8")
        file.pos += size
        ret = super(Schema, cls).read(file)
        assert file.pos - op == l, 'failed: {} {} {}'.format(file.pos, op, l)
        return Schema_(name, ret)


    @classmethod
    def write(cls, file, data):
        writer = karabo.hash.BinaryWriter()
        h = writer.write(data.hash)
        s = data.name.encode('utf8')
        print(len(h), len(s))
        file.writeFormat('I', len(h) + len(s) + 1)
        file.writeFormat('B', len(s))
        file.file.write(s)
        file.file.write(h)


class Schema_(Special):
    hashtype = Schema


    def __init__(self, name=None, hash=None, rules=None):
        self.name = name
        if hash is None:
            self.hash = karabo.hash.Hash()
        else:
            self.hash = hash
        self.rules = rules


    def copy(self, other):
        self.hash = karabo.hash.Hash()
        self.hash.merge(other.hash)


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
