from enums import AccessMode
import hash
from registry import Registry

from struct import unpack, calcsize
import numpy


class Simple(object):
    @classmethod
    def read(cls, file):
        return cls.numpy(*file.readFormat(cls.format))


    @classmethod
    def fromstring(cls, s):
        return cls.numpy(s)


    @classmethod
    def write(cls, file, data):
        file.writeFormat(cls.format, data)


class Integer(Simple):
    pass


class Number(Simple):
    pass


class Vector(object):
    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        return numpy.array([super(Vector, cls).read(file) for i in range(size)])


    @classmethod
    def fromstring(cls, s):
        return numpy.array([cls.numpy(x) for x in s.split(',')])


    @classmethod
    def write(cls, file, data):
        file.writeFormat('I', len(data))
        for d in data:
            super(Vector, cls).write(file, d)


class Type(Registry):
    types = [ ]
    fromname = { }
    strs = { }


    @classmethod
    def register(cls, name, dict):
        cls.number = len(cls.types)
        cls.types.append(cls)
        s = ''
        lastlower = False
        for c in cls.__name__:
            if c.isupper() and lastlower:
                s += '_'
            s += c.capitalize()
            lastlower = c.islower()
        cls.fromname[s.rstrip('_')] = cls
        if 'numpy' in dict:
            cls.strs[cls.numpy().dtype.str] = cls


class Bool(Type):
    @classmethod
    def read(cls, file):
        return Int8.read(file) != 0


    @staticmethod
    def fromstring(s):
        return bool(int(s))


class VectorBool(Type):
    pass


class Char(Simple, Type):
    format = "c"
    numpy = numpy.str_


class VectorChar(Vector, Char):
    pass


class Int8(Integer, Type):
    format = "b"
    numpy = numpy.int8


class VectorInt8(Vector, Int8):
    pass


class UInt8(Integer, Type):
    format = "B"
    numpy = numpy.uint8


class VectorUInt8(Vector, UInt8):
    pass


class Int16(Integer, Type):
    format = "h"
    numpy = numpy.int16


class VectorInt16(Vector, Int16):
    pass


class UInt16(Integer, Type):
    format = "H"
    numpy = numpy.uint16


class VectorUInt16(Vector, UInt16):
    pass


class Int32(Integer, Type):
    format = "i"
    numpy = numpy.int32


class VectorInt32(Vector, Int32):
    pass


class UInt32(Integer, Type):
    format = "I"
    numpy = numpy.uint32


class VectorUInt32(Vector, UInt32):
    pass


class Int64(Integer, Type):
    format = "q"
    numpy = numpy.int64


class VectorInt64(Vector, Int64):
    pass


class UInt64(Integer, Type):
    format = "Q"
    numpy = numpy.uint64


class VectorUInt64(Vector, UInt64):
    pass


class Float(Number, Type):
    format = "f"
    numpy = numpy.float32


class VectorFloat(Vector, Float):
    pass


class Double(Number, Type):
    format = "d"
    numpy = numpy.float64


class VectorDouble(Vector, Double):
    pass


class ComplexFloat(Number, Type):
    format = "ff"
    numpy = numpy.complex64


class VectorComplexFloat(Vector, ComplexFloat):
    pass


class ComplexDouble(Number, Type):
    format = "dd"
    numpy = numpy.complex128


class VectorComplexDouble(Vector, ComplexDouble):
    pass


class String(Type):
    @staticmethod
    def read(file):
        size, = file.readFormat('I')
        return file.file.read(size)


    @staticmethod
    def fromstring(s):
        return unicode(s)


    @classmethod
    def write(cls, file, data):
        data = str(data)
        file.writeFormat('I', len(data))
        file.file.write(data)


class VectorString(Vector, String):
    @staticmethod
    def fromstring(s):
        return unicode(s).split(',')


    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        return [super(Vector, cls).read(file) for i in xrange(size)]


class Hash(Type):
    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        ret = hash.Hash()
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
            for ak, av in data[k, ...]:
                file.writeKey(ak)
                file.writeType(av)
                file.writeData(av)
            file.writeData(v)


class VectorHash(Type):
    pass


class PtrBool(Type):
    pass


class PtrChar(Type):
    pass


class PtrInt8(Type):
    pass


class PtrUInt8(Type):
    pass


class PtrInt16(Type):
    pass


class PtrUInt16(Type):
    pass


class PtrInt32(Type):
    pass


class PtrUInt32(Type):
    pass


class PtrInt64(Type):
    pass


class PtrUInt64(Type):
    pass


class PtrFloat(Type):
    pass


class PtrDouble(Type):
    pass


class PtrComplexFloat(Type):
    pass


class PtrComplexDouble(Type):
    pass


class PtrString(Type):
    pass


class Schema(Hash):
    @classmethod
    def read(cls, file):
        file.readFormat('I') # ignore length
        size, = file.readFormat('Q')
        name = file.file.read(size)
        ret = super(Schema, cls).read(file)
        return hash.Schema(name, ret)


class VectorSchema(Type):
    pass


class Any(Type):
    pass


class None_(Type):
    pass


class VectorNone(Type):
    pass


class Unknown(Type):
    pass


class Simple_(Type):
    pass


class Sequence(Type):
    pass


class Pointer(Type):
    pass
