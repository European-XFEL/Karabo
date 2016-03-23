""" This module contains the type hierarchy implied by the Karabo hash.

This file closely corresponds to karabo.util.ReferenceType.

The C++ types are mostly implemented by using the corresponding numpy type.
"""

from asyncio import async, iscoroutinefunction, coroutine, get_event_loop
import base64
from collections import OrderedDict
from enum import Enum
from io import BytesIO
import numbers
from struct import pack, unpack, calcsize
import sys
from xml.etree import ElementTree

import numpy as np

from . import basetypes
from .enums import (AccessLevel, AccessMode, Assignment, MetricPrefix,
                    NodeType, Unit)
from .exceptions import KaraboError
from .registry import Registry


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
        ret = np.frombuffer(file.data, cls.numpy, 1, file.pos)[0]
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
            ret = super().cast(other)
        elif isinstance(other, self.numpy):
            ret = other
        else:
            ret = self.numpy(other)
        if (self.minExc is not None and ret <= self.minExc or
                self.minInc is not None and ret < self.minInc or
                self.maxExc is not None and ret >= self.maxExc or
                self.maxInc is not None and ret > self.maxInc):
            raise ValueError("value {} of {} not in allowed range".
                             format(ret, self.key))
        return ret


class Integer(Simple, Enumable):
    def getMinMax(self):
        info = np.iinfo(self.numpy)
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
    absoluteError = Attribute()
    relativeError = Attribute()

    def getMinMax(self):
        info = np.finfo(self.numpy)
        min = self.minExc
        if min is None:
            min = self.minInc
        else:
            min = min * (1 + np.sign(min) * info.eps) + info.tiny
        if min is None:
            min = info.min

        max = self.maxExc
        if max is None:
            max = self.maxInc
        else:
            max = max * (1 - np.sign(max) * info.eps) - info.tiny
        if max is None:
            max = info.max

        return min, max


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
        return Hash()

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            instance._use()
            if self.key not in instance.__dict__:
                raise AttributeError(
                    "attribute '{}' has not been set".format(self.key))
            return instance.__dict__[self.key]

    def __set__(self, instance, value):
        v = self.cast(value)
        instance.setValue(self, v)

    def setter(self, instance, value):
        """this is to be called if the value is changed from the outside"""
        setattr(instance, self.key, value)

    @coroutine
    def setter_async(self, instance, value):
        self.setter(instance, self.cast(value))

    def checkedSet(self, instance, value):
        if self.accessMode is not AccessMode.RECONFIGURABLE:
            msg = 'property "{}" is not reconfigurable'.format(self.key)
            raise KaraboError(msg)
        elif (self.allowedStates is not None and
              instance.state not in self.allowedStates):
            msg = 'setting "{}" is not allowed in state "{}"'.format(
                self.key, instance.state)
            raise KaraboError(msg)
        else:
            return self.setter_async(instance, value)


class Slot(Descriptor):
    iscoroutine = None

    def parameters(self):
        ret = super(Slot, self).parameters()
        ret["nodeType"] = NodeType.Node
        ret["displayType"] = "Slot"
        return ret

    def asHash(self, other):
        return Hash()

    def cast(self, other):
        return Hash()

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            def inner(device):
                return self.method(device)
            inner.slot = self.inner
            return inner.__get__(instance, owner)

    def inner(self, device, message, args):
        if (self.allowedStates is not None and
                device.state not in self.allowedStates):
            msg = 'calling slot "{}" not allowed in state "{}"'.format(
                self.key, device.state)
            device._ss.reply(message, msg)
            raise KaraboError(msg)
        if self.iscoroutine or iscoroutinefunction(self.method):
            coro = self.method(device)
        else:
            coro = get_event_loop().start_thread(self.method, device)
        def inner():
            try:
                device._ss.reply(message, (yield from coro))
            except Exception as e:
                _, exc, tb = sys.exc_info()
                device._onException(self, exc, tb)
                device._ss.reply(message, str(e))
        return async(inner())

    def method(self, device):
        return self.themethod(device)

    def setter(self, instance, value):
        pass  # nothing to set in a slot

    def __call__(self, method):
        if self.description is None:
            self.description = method.__doc__
        self.iscoroutine = iscoroutinefunction(method)
        self.themethod = method
        return self


class Special(object):
    """These is the base class for special types, which neither have
    a python nor numpy equivalent"""


class Type(Descriptor, Registry):
    """This is the base class for all types in the Karabo type hierarchy. """
    unitSymbol = Attribute(Unit.NOT_ASSIGNED)
    metricPrefixSymbol = Attribute(MetricPrefix.NONE)
    enum = None

    types = [None] * 51
    fromname = { }
    strs = { }

    options = Attribute()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        if self.options is not None:
            self.options = [self.cast(o) for o in self.options]
        self.dimensionality = basetypes.QuantityValue(
            1, unit=self.unitSymbol).dimensionality

    @classmethod
    def hashname(cls):
        return cls._hashname

    @classmethod
    def register(cls, name, dict):
        super(Type, cls).register(name, dict)

        if "number" in dict:
            cls.types[cls.number] = cls
            s = ''
            lastlower = False
            for c in cls.__name__:
                if c.isupper() and lastlower:
                    s += '_'
                s += c.capitalize()
                lastlower = c.islower()
            cls._hashname = s.rstrip('_')
            cls.fromname[cls.hashname()] = cls
        if 'numpy' in dict:
            cls.strs[cls.numpy().dtype.str] = cls

    @classmethod
    def toString(cls, data):
        return str(data)

    def asHash(self, data):
        return self.cast(data)

    def parameters(self):
        ret = super(Type, self).parameters()
        ret["nodeType"] = NodeType.Leaf
        ret["valueType"] = self.hashname()
        return ret

    def __call__(self, method):
        if self.description is None:
            self.description = method.__doc__
        self.setter = method
        return self


class Vector(Type):
    @classmethod
    def register(cls, name, dict):
        super(Vector, cls).register(name, dict)
        if "basetype" in dict:
            cls.basetype.vectortype = cls

    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        return (cls.basetype.read(file) for i in range(size))

    @classmethod
    def write(cls, file, data):
        file.writeFormat('I', len(data))
        for d in data:
            cls.basetype.write(file, d)


class NumpyVector(Vector):
    vstrs = { }

    @classmethod
    def register(cls, name, dict):
        super(NumpyVector, cls).register(name, dict)
        cls.vstrs[cls.basetype.numpy().dtype.str] = cls

    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        ret = np.frombuffer(file.data, cls.basetype.numpy, size, file.pos)
        file.pos += cls.basetype.numpy().itemsize * size
        return ret

    @classmethod
    def fromstring(cls, s):
        if s:
            return np.fromstring(s, sep=",", dtype=cls.basetype.numpy)
        else:
            return np.array([], dtype=cls.basetype.numpy)

    @classmethod
    def toString(cls, data):
        return ",".join(str(x) for x in data)

    def cast(self, other):
        if isinstance(other, np.ndarray) and \
                other.dtype == self.basetype.numpy:
            ret = other
        else:
            ret = np.array(other, dtype=self.basetype.numpy)
        assert ret.ndim == 1, "can only treat one-dimensional vectors"
        return ret


class Bool(Type):
    number = 0
    numpy = np.bool_

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


class VectorBool(NumpyVector):
    basetype = Bool
    number = 1

    @classmethod
    def toString(cls, data):
        return ",".join(str(int(i)) for i in data)


class Char(Simple, Type):
    number = 2
    numpy = np.uint8 # actually not used, for convenience only

    @staticmethod
    def read(file):
        file.pos += 1
        return _Byte(file.data[file.pos - 1:file.pos].decode("ascii"))

    @classmethod
    def toString(cls, data):
        return data

    @classmethod
    def fromstring(self, s):
        return s

    @classmethod
    def write(cls, file, data):
        assert len(data) == 1
        file.file.write(data.encode("ascii"))

    def cast(self, other):
        if len(bytes(other)) == 1:
            return _Byte(other)


class _Byte(Special, str):
    """This represents just one byte, so that we can distinguish
    CHAR and VECTOR_CHAR."""
    hashtype = Char

    def __repr__(self):
        return "${:x}".format(ord(self))


class VectorChar(Vector):
    """A VectorChar is simply some binary data in memory. The corresponding
    python data type is bytes. Make sure you don't use str for strings,
    as this will result in the hash creating a VectorChar, and the C++ will
    not be happy about this."""
    basetype = Char
    number = 3

    @staticmethod
    def read(file):
        size, = file.readFormat('I')
        file.pos += size
        return bytes(file.data[file.pos - size:file.pos])

    @classmethod
    def toString(cls, data):
        return base64.b64encode(data).decode("ascii")

    @classmethod
    def fromstring(self, s):
        return base64.b64decode(s)

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
    numpy = np.int8


class VectorInt8(NumpyVector):
    basetype = Int8
    number = 5


class UInt8(Integer, Type):
    number = 6
    format = "B"
    numpy = np.uint8


class VectorUInt8(NumpyVector):
    basetype = UInt8
    number = 7


class Int16(Integer, Type):
    number = 8
    format = "h"
    numpy = np.int16


class VectorInt16(NumpyVector):
    basetype = Int16
    number = 9


class UInt16(Integer, Type):
    number = 10
    format = "H"
    numpy = np.uint16


class VectorUInt16(NumpyVector):
    basetype = UInt16
    number = 11


class Int32(Integer, Type):
    number = 12
    format = "i"
    numpy = np.int32


class VectorInt32(NumpyVector):
    basetype = Int32
    number = 13


class UInt32(Integer, Type):
    number = 14
    format = "I"
    numpy = np.uint32


class VectorUInt32(NumpyVector):
    basetype = UInt32
    number = 15


class Int64(Integer, Type):
    number = 16
    format = "q"
    numpy = np.int64


class VectorInt64(NumpyVector):
    basetype = Int64
    number = 17


class UInt64(Integer, Type):
    number = 18
    format = "Q"
    numpy = np.uint64


class VectorUInt64(NumpyVector):
    basetype = UInt64
    number = 19


class Float(Number, Type):
    number = 20
    format = "f"
    numpy = np.float32


class VectorFloat(NumpyVector):
    basetype = Float
    number = 21


class Double(Number, Type):
    number = 22
    format = "d"
    numpy = np.float64


class VectorDouble(NumpyVector):
    basetype = Double
    number = 23


class ComplexFloat(Number, Type):
    number = 24
    format = "ff"
    numpy = np.complex64

    @classmethod
    def fromstring(cls, s):
        return complex(*[float(n) for n in s[1:-1].split(',')])

    @classmethod
    def toString(cls, data):
        return "({},{})".format(data.real, data.imag)

    @classmethod
    def write(cls, file, data):
        file.writeFormat(cls.format, data.real, data.imag)


class VectorComplexFloat(NumpyVector):
    basetype = ComplexFloat
    number = 25


class ComplexDouble(Number, Type):
    number = 26
    format = "dd"
    numpy = np.complex128

    @classmethod
    def fromstring(cls, s):
        return complex(*[float(n) for n in s[1:-1].split(',')])

    @classmethod
    def toString(cls, data):
        return "({},{})".format(data.real, data.imag)

    @classmethod
    def write(cls, file, data):
        file.writeFormat(cls.format, data.real, data.imag)


class VectorComplexDouble(NumpyVector):
    basetype = ComplexDouble
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


class VectorString(Vector):
    basetype = String
    number = 29

    @staticmethod
    def fromstring(s):
        if not s:
            return StringList()
        return StringList(ss.strip() for ss in s.split(','))

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


class HashType(Type):
    number = 30

    @classmethod
    def read(cls, file):
        size, = file.readFormat('I')
        ret = Hash()
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


class VectorHash(Vector):
    basetype = HashType
    number = 31

    @classmethod
    def read(cls, file):
        return list(super(VectorHash, cls).read(file))

    def cast(self, other):
        ht = HashType()
        return [ht.cast(o) for o in other]


class SchemaHashType(HashType):
    number = 47

    @classmethod
    def read(cls, file):
        l, = file.readFormat('I') # ignore length
        op = file.pos
        size, = file.readFormat('B')
        name = str(file.data[file.pos:file.pos + size], "utf8")
        file.pos += size
        ret = super(SchemaHashType, cls).read(file)
        assert file.pos - op == l, 'failed: {} {} {}'.format(file.pos, op, l)
        return Schema(name, hash=ret)

    @classmethod
    def write(cls, file, data):
        for p in data.hash.paths():
            nodeType = NodeType(data.hash[p, "nodeType"])
            if nodeType is NodeType.Leaf:
                assert not data.hash[p], "no proper leaf: {}".format(p)
            else:
                assert isinstance(data.hash[p], Hash), \
                    "no proper node: {}".format(p)
        writer = BinaryWriter()
        h = writer.write(data.hash)
        s = data.name.encode('utf8')
        file.writeFormat('I', len(h) + len(s) + 1)
        file.writeFormat('B', len(s))
        file.file.write(s)
        file.file.write(h)

    @classmethod
    def toString(cls, data):
        return data.name + ":" + data.hash.encode("XML").decode("utf8")

    @classmethod
    def fromstring(cls, s):
        name, xml = s.split(":", 1)
        return Schema(name, hash=Hash.decode(xml, "XML"))

    @classmethod
    def hashname(cls):
        return 'SCHEMA'


class Schema(Special):
    hashtype = SchemaHashType

    def __init__(self, name=None, rules=None, hash=None):
        self.name = name
        if hash is None:
            self.hash = Hash()
        else:
            self.hash = hash
        self.rules = rules

    def copy(self, other):
        self.hash = Hash()
        self.hash.merge(other.hash)

    def keyHasAlias(self, key):
        return "alias" in self.hash[key, ...]

    def getAliasAsString(self, key):
        return self.hash[key, "alias"]

    def getKeyFromAlias(self, alias):
        for k in self.hash.paths():
            if alias == self.hash[k, ...].get("alias", None):
                return k

    def getValueType(self, key):
        return Type.fromname[self.hash[key, "valueType"]]

    def __repr__(self):
        return "Schema('{}', {})".format(self.name, self.hash)


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


def _gettype(data):
    try:
        if isinstance(data, np.ndarray):
            return NumpyVector.vstrs[data.dtype.str]
        else:
            return Type.strs[data.dtype.str]
    except AttributeError:
        if hasattr(data, "hashtype"):
            return data.hashtype
        elif isinstance(data, bool):
            return Bool
        elif isinstance(data, Enum):
            return Int32
        elif isinstance(data, numbers.Integral):
            return Int32
        elif isinstance(data, numbers.Real):
            return Double
        elif isinstance(data, numbers.Complex):
            return ComplexDouble
        elif isinstance(data, bytes):
            return VectorChar
        elif isinstance(data, str):
            return String
        elif isinstance(data, list):
            if data:
                return _gettype(data[0]).vectortype
            else:
                return VectorString
        elif data is None:
            return None_
        else:
            raise TypeError('unknown datatype {}'.format(data.__class__))


class Element(object):
    text = property(lambda self: None, lambda self, value: None)
    tail = text

    def __init__(self, tag, attrs={}):
        self.tag = tag
        if "KRB_Artificial" in attrs:
            self.artificial = True
        self.type = attrs.get("KRB_Type")
        def parse(vv):
            k, v = vv.split(":", 1)
            return Type.fromname[k[4:]].fromstring(v)
        self.attrs = {k: parse(v) for k, v in attrs.items()
                      if k[:4] != "KRB_"}


    def items(self):
        yield "KRB_Type", self.hashname()
        for k, v in self.attrs.items():
            t = _gettype(v)
            yield k, 'KRB_{}:{}'.format(t.hashname(), t.toString(v))


class SimpleElement(Element):
    def __len__(self):
        return 0

    def append(self, elem):
        raise RuntimeError("no append to simple element")

    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self

    def __iter__(self):
        return
        yield

    @property
    def text(self):
        try:
            return _gettype(self.data).toString(self.data)
        except AttributeError:
            return None

    @text.setter
    def text(self, value):
        try:
            self.data = Type.fromname[self.type].fromstring(value)
        except:
            raise


    def hashname(self):
        return _gettype(self.data).hashname()


class HashElement(Element):
    def __init__(self, tag, attrs={}):
        Element.__init__(self, tag, attrs)
        self.children = Hash()

    def __len__(self):
        return len(self.children)

    def append(self, elem):
        OrderedDict.__setitem__(self.children, elem.tag, elem)

    def __iter__(self):
        for e in self.children:
            yield OrderedDict.__getitem__(self.children, e)

    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self
        for e in self.children:
            for e in OrderedDict.__getitem__(self.children, e).iter(tag):
                yield e

    @property
    def data(self):
        return self.children


    def hashname(self):
        return "HASH"


class ListElement(Element):
    def __init__(self, tag, attrs={}):
        Element.__init__(self, tag, attrs)
        self.children = [ ]


    def __len__(self):
        return len(self.children)


    def append(self, elem):
        self.children.append(elem)


    def __iter__(self):
        return iter(self.children)


    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self
        for e in self.children:
            for ee in e.iter(tag):
                yield ee


    @property
    def data(self):
        return [e.data for e in self.children]


    @data.setter
    def data(self, value):
        self.children = [ ]
        for c in value:
            e = HashElement('KRB_Item')
            e.children = c
            self.children.append(e)


    def hashname(self):
        return "VECTOR_HASH"

class TableElement(Element):
    def __init__(self, tag, attrs={}):
        Element.__init__(self, tag, attrs)
        self.children = [ ]
 
 
    def __len__(self):
        return len(self.children)
 
 
    def append(self, elem):
        self.children.append(elem)
 
 
    def __iter__(self):
        return iter(self.children)
 
 
    def iter(self, tag=None):
        if tag == "*":
            tag = None
        if tag is None or self.tag == tag:
            yield self
        for e in self.children:
            for ee in e.iter(tag):
                yield ee
 
 
    @property
    def data(self):
        return [e.data for e in self.children]
 
 
    @data.setter
    def data(self, value):
        self.children = [ ]
        for c in value:
            e = HashElement('KRB_Item')
            e.children = c
            self.children.append(e)
 
 
    def hashname(self):
        return "VECTOR_HASH"

class Hash(OrderedDict):
    """This is the serialization data structure of Karabo

    Every data that gets transfered over the network or saved to file
    by Karabo is in this format.

    It is mostly an extended :class:`dict`.

    The bit difference to normal python containers is the dot-access method.
    The hash has a built-in knowledge about it containing itself. Thus,
    one can access subhashes by ``hash['key.subhash']``.

    The other speciality are attributes. In python, these can be accessed
    using a second parameter to the brackets, as in
    ``hash['key', 'attribute']``.

    All attributes at the same time can be accessed by ``hash['key', ...]``."""

    hashtype = HashType

    def __init__(self, *args):
        if len(args) == 1:
            if isinstance(args[0], Hash):
                OrderedDict.__init__(self)
                for k, v, a in args[0].iterall():
                    self[k] = v
                    self[k, ...] = dict(a)
            else:
                OrderedDict.__init__(self, args[0])
        else:
            OrderedDict.__init__(self)
            for k, v in zip(args[::2], args[1::2]):
                self[k] = v

    def _path(self, path, auto=False):
        path = path.split(".")
        s = self
        for p in path[:-1]:
            if auto and p not in s:
                OrderedDict.__setitem__(s, p, HashElement(p))
            s = s[p]
        if not isinstance(s, Hash):
            raise KeyError(path)
        return s, path[-1]


    def _get(self, path, auto=False):
        return OrderedDict.__getitem__(*self._path(path, auto))


    def __repr__(self):
        r = ', '.join('{}{!r}: {!r}'.format(k, self[k, ...], self[k])
                      for k in self)
        return '<' + r + '>'


    def __setitem__(self, item, value):
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                self._get(key).attrs = value
            else:
                self._get(key).attrs[attr] = value
        else:
            s, p = self._path(str(item), True)
            if p in s:
                attrs = s[p, ...]
            else:
                attrs = { }
            if isinstance(value, Hash):
                elem = HashElement(p)
                elem.children = value
            elif (isinstance(value, list) and
                  value and isinstance(value[0], Hash)):
                elem = ListElement(p)
                elem.data = value
            else:
                elem = SimpleElement(p)
                elem.data = value
            elem.attrs = attrs
            OrderedDict.__setitem__(s, p, elem)

    def __getitem__(self, item):
        if isinstance(item, tuple):
            key, attr = item
            if attr is Ellipsis:
                return self._get(key).attrs
            else:
                return self._get(key).attrs[attr]
        else:
            return self._get(item).data

    def __delitem__(self, item):
        if isinstance(item, tuple):
            key, attr = item
            del self._get(key).attrib[attr]
        else:
            OrderedDict.__delitem__(*self._path(item))


    def __contains__(self, key):
        try:
            self._get(key)
            return True
        except KeyError:
            return False


    def iterall(self):
        """ Iterate over key, value and attributes

        This behaves like the ``items()`` method of python :class:`dict`,
        just that it yields not only key and value but also the attributes
        for it. """
        for k in self:
            yield k, self[k], self[k, ...]


    def merge(self, other, attribute_policy='merge'):
        """Merge the hash other into this hash.

        If the *attribute_policy* is ``'merge'``, the attributes from the other
        hash are merged with the existing ones, otherwise they are overwritten.
        """
        merge = attribute_policy == "merge"
        for k, v in other.items():
            if isinstance(v, Hash):
                if k not in self or self[k] is None:
                    self[k] = Hash()
                self[k].merge(v, attribute_policy)
            else:
                self[k] = v
            if merge:
                self[k, ...].update(other[k, ...])
            else:
                self[k, ...] = other[k, ...].copy()


    def get(self, item, default=None):
        try:
            return self[item]
        except KeyError:
            return default

    def set(self, item, value):
        self[item] = value

    def setAttribute(self, item, key, value):
        self[item, key] = value

    def getAttribute(self, item, key):
        return self[item, key]


    def getAttributes(self, item):
        return self[item, ...]

    def has(self, item):
        return item in self

    def getKeys(self, keys=None):
        if keys is None:
            return list(self.keys())
        return keys.extend(list(self.keys()))

    def hasAttribute(self, item, key):
        return key in self[item, ...]

    def erase(self, key):
        del self[key]

    def paths(self):
        ret = [ ]
        for k, v in self.items():
            if isinstance(v, Hash):
                ret.extend(k + '.' + kk for kk in v.paths())
            ret.append(k)
        return ret

    def empty(self):
        return len(self) == 0

    def encode(self, format):
        writer = {"XML": XMLWriter, "Bin": BinaryWriter}.get(format, format)()
        return writer.write(self)

    @staticmethod
    def decode(data, format):
        reader = {"XML": XMLParser, "Bin": BinaryParser}.get(format, format)()
        return reader.read(data)

class HashMergePolicy:
    MERGE_ATTRIBUTES = "merge"
    REPLACE_ATTRIBUTES = "replace"


class XMLParser(object):
    last = None


    def factory(self, tag, attrs):
        krb_type = attrs.get("KRB_Type", "")
        if krb_type == "HASH":
            return HashElement(tag, attrs)
        elif krb_type == "VECTOR_HASH":
            return ListElement(tag, attrs)
        elif tag == "KRB_Item":
            return HashElement("", attrs)
        else:
            self.closelast()
            self.last = SimpleElement(tag, attrs)
            return self.last


    def closelast(self):
        # while parsing ElementTree does not set text if the is none...
        if self.last is not None and not hasattr(self.last, "data"):
            self.last.text = ""


    def read(self, data):
        """Parse the XML in the buffer data and return the hash"""
        target = ElementTree.TreeBuilder(element_factory=self.factory)
        parser = ElementTree.XMLParser(target=target)
        parser.feed(data)
        root = target.close()
        self.closelast()
        if hasattr(root, "artificial"):
            return root.children
        else:
            ret = Hash()
            OrderedDict.__setitem__(ret, root.tag, root)
            return ret


class Writer(object):
    def write(self, data):
        """Return the written data as a string"""
        self.file = BytesIO()
        try:
            self.writeToFile(data, self.file)
            return self.file.getvalue()
        finally:
            self.file.close()


class XMLWriter(Writer):
    def writeToFile(self, hash, file):
        """Write the hash to the file in binary format"""
        if len(hash) == 1 and isinstance(list(hash.values())[0], Hash):
            e = OrderedDict.__getitem__(hash, list(hash.keys())[0])
        else:
            e = HashElement("root")
            e.attrs = dict(KRB_Artificial="")
            e.children = hash
        et = ElementTree.ElementTree(e)
        et.write(file)


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


class BinaryWriter(Writer):
    def writeFormat(self, fmt, *data):
        s = pack(fmt, *data)
        self.file.write(s)

    def writeKey(self, key):
        key = key.encode('utf8')
        self.writeFormat('B', len(key))
        self.file.write(key)


    def writeType(self, data):
        type = _gettype(data)
        self.writeFormat('I', type.number)


    def writeData(self, data):
        type = _gettype(data)
        type.write(self, data)


    def writeToFile(self, data, file):
        self.file = file
        HashType.write(self, data)


def saveToFile(hash, fn):
    w = XMLWriter()
    with open(fn, 'wb') as out:
        w.writeToFile(hash, out)
