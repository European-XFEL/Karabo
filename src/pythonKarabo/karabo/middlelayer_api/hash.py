""" This module contains the type hierarchy implied by the Karabo hash.

This file closely corresponds to karabo.util.ReferenceType.

The C++ types are mostly implemented by using the corresponding numpy type.
"""

from asyncio import async, coroutine, get_event_loop
import base64
from collections import OrderedDict
from enum import Enum
from functools import partial, wraps
from io import BytesIO
import numbers
import pint
from struct import pack, unpack, calcsize
import sys
import weakref
from xml.etree import ElementTree

import numpy as np

from karabo.common.states import State
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

    def toDataAndAttrs(self, data):
        h, attrs = super().toDataAndAttrs(data)
        if self.enum is not None:
            return data.value, attrs
        else:
            return h, attrs

    def toKaraboValue(self, data, strict=True):
        if not strict and not isinstance(data, self.enum):
            data = self.enum(data)
        return basetypes.EnumValue(data, descriptor=self)


class Simple(object):
    """This is the base for all numeric types

    It features the minimum and maximum for its values given
    in ``minExc``, ``maxExc``, ``minInc``, and ``maxInc``, where
    ``min`` and ``max`` stand for miniumum and maximum, while ``Exc``
    and ``Inc`` stand for exclusive and inclusive, respectively.

    Do not use inclusive and exclusive for the same limit at the same
    time.
    """

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
        self.check(ret)
        return ret

    def check(self, ret):
        if (self.minExc is not None and ret <= self.minExc or
                self.minInc is not None and ret < self.minInc or
                self.maxExc is not None and ret >= self.maxExc or
                self.maxInc is not None and ret > self.maxInc):
            raise ValueError("value {} of {} not in allowed range".
                             format(ret, self.key))

    def toKaraboValue(self, data, strict=True):
        if self.enum is not None:
            return Enumable.toKaraboValue(self, data, strict)
        if not strict or not self.dimensionality or isinstance(data, str):
            data = basetypes.QuantityValue(data, descriptor=self)
        elif not isinstance(data, basetypes.QuantityValue):
            raise pint.DimensionalityError("no dimension", self.dimensionality)
        if data.units != self.units:
            data = data.to(self.units)
            data.descriptor = self
        self.check(data.magnitude)
        return data

    def getMinMax(self):
        """Return a tuple (minimum, maximum) for this value

        This are the minimum and maximum value this type can take.
        This is based both on the definition of the type, as well as
        on the attributes ``minInc``, ``maxInc``, ``minExc``,
        ``maxExc``. All information can be condensed in just two
        values as it is futile to specify both inclusive and exclusive
        values for the same limit.
        """
        raise NotImplementedError


class Integer(Simple, Enumable):
    """The base class for all integers"""

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

    def toKaraboValue(self, data, strict=True):
        if self.enum is not None:
            return Enumable.toKaraboValue(self, data, strict)
        ret = Simple.toKaraboValue(self, data, strict)
        return basetypes.QuantityValue(int(ret.magnitude), unit=ret.units,
                                       descriptor=self,
                                       timestamp=ret.timestamp)


class Number(Simple):
    """The base class for all floating-point types"""

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
    """This is the base class for all descriptors in Karabo

    Descriptors describe the content of a device property. The description
    is done in their attributes, which come from a fixed defined set,
    described in :ref:`howto-properties`.

    It may be useful to note that instances of this class do *not* contain
    any data, instead they are *describing* which values a device property
    may take, they are actually :ref:`descriptors <python:descriptors>` in the
    Python sense. They are given as keyword arguments upon initialization,
    for example::

        class MyDevice(Device):
            # Int32 is a subclass of Descriptor
            # displayedName and defaultValue are the attributes
            count = Int32(displayedName="electron count", defaultValue=5)
    """

    displayedName = Attribute()
    alias = Attribute()
    description = Attribute()
    defaultValue = Attribute()
    accessMode = Attribute(AccessMode.RECONFIGURABLE)
    assignment = Attribute(Assignment.OPTIONAL)
    requiredAccessLevel = Attribute(AccessLevel.OBSERVER)
    displayType = Attribute()
    allowedStates = None

    def __init__(self, strict=True, key="(unknown key)",
                 allowedStates=None, **kwargs):
        """Create a new descriptor with appropriate attributes

        The attributes are given as keyword arguments. If we define
        those in our code, we assume that all attributes are correctly
        given, or raise an error. If we are parsing a schema that came
        over the network, we are not so *strict* and silently discard
        unknown attributes, and properly set the enum type for those
        we find.
        """
        self.key = key
        for k, v in kwargs.items():
            attr = getattr(self.__class__, k, None)
            if isinstance(attr, Attribute):
                if attr.default is None:
                    setattr(self, k, v)
                else:
                    converted = type(attr.default)(v)
                    if strict and converted != v:
                        raise TypeError(
                            '{} got attribute {} with value "{}" of incorrect '
                            'type {}'.format(self.__class__.__name__, k, v,
                                             type(v)))
                    setattr(self, k, converted)
            elif strict:
                raise TypeError("{} got unexpected keyword argument: {}".
                                format(self.__class__.__name__, k))
        if allowedStates is not None:
            if strict:
                self.allowedStates = set(allowedStates)
                if not all((isinstance(s, State) for s in self.allowedStates)):
                    raise TypeError(
                        'allowedStates must contain States, not "{}"'.
                        format(allowedStates))
            else:
                self.allowedStates = set((State(s) for s in allowedStates))

    def toSchemaAndAttrs(self, device, state):
        """return schema for device in state

        This returns the Hash representing this descriptor in a Schema, as well
        as the attributes that have to be set for it.

        if device is None, the class' Schema is returned, otherwise the
        device's. If state is not None, only the parts of the schema available
        in the given state are returned.
        """
        attrs = ((name, getattr(self, name)) for name in dir(type(self))
                 if isinstance(getattr(type(self), name), Attribute))
        attrs = ((name, value.value if isinstance(value, Enum) else value)
                 for name, value in attrs)
        attrs = {name: attr for name, attr in attrs if attr is not None}
        if self.allowedStates is not None:
            attrs["allowedStates"] = [s.value for s in self.allowedStates]
        return Hash(), attrs

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
        instance.setValue(self, self.toKaraboValue(value))

    def setter(self, instance, value):
        """This is called when the value is changed from the outside

        One may override this method if something special should happen.
        This method must return None (e.g. by having no return statement
        at all).
        """
        setattr(instance, self.key, value)

    def _setter(self, instance, value):
        """Return a list with callables to be called to set the value

        `value` still is the bare Hash value, as it came from the network.
        The actual setting is done by `setter` later.
        """
        return [partial(self.setter, instance,
                        self.toKaraboValue(value, strict=False))]

    def initialize(self, instance, value):
        """This is called when the value is initialized

        One may override this method if something special should happen,
        otherwise `setter` is called. This method must return None
        (e.g. by having no return statement at all).
        """
        return self.setter(instance, value)

    def _initialize(self, instance, value):
        """Initialize values and return a list of coroutines to be called

        `value` still is the bare Hash value, as it came from the network.
        `initialize` is called with corresponding `KaraboValue`.
        """
        ret = self.initialize(instance,
                              self.toKaraboValue(value, strict=False))
        if ret is None:
            return []
        else:
            return [ret]

    def checkedSet(self, instance, value):
        """Check whether it is allowed and return setters

        This checks whether it is allowed to set the value and if so
        returns an iterable with setter that need to be called to set the
        value.
        """
        if self.accessMode is not AccessMode.RECONFIGURABLE:
            msg = 'property "{}" is not reconfigurable'.format(self.key)
            raise KaraboError(msg)
        elif (self.allowedStates is not None and
              instance.state not in self.allowedStates):
            msg = 'Setting "{}" is not allowed in state "{}"'.format(
                self.key, instance.state)
            raise KaraboError(msg)
        else:
            return self._setter(instance, value)

    def checkedInit(self, instance, value=None):
        """Check whether it is allowed and initialze

        This checks whether it is allowed to initialize the value and
        returns an iterable with coroutines which may be needed for
        delayed initialization.
        """
        # initial values for READONLY attributes are ignored
        if value is None or self.accessMode is AccessMode.READONLY:
            if self.assignment is Assignment.MANDATORY:
                raise KaraboError(
                    'assignment is mandatory for "{}"'.format(self.key))
            if self.defaultValue is None:
                return []
            return self._initialize(instance, self.defaultValue)
        return self._initialize(instance, value)

    def toDataAndAttrs(self, value):
        """Split value in bare data and the attributes that go with it

        Return ``data`` in a format suitable to be put into a ``Hash``,
        and the attributes (namely: the timestamp) that should go with it.
        """
        raise NotImplementedError


class Slot(Descriptor):
    '''Define a slot callable from the outside

    This is a decorator for methods that should be callable over the network.
    The cannot have parameters. All the arguments can be given in the same
    way as for descriptors. A docstring will be taken as Karabo description::

        class SomeDevice(Device):
            @Slot(displayedName="Do something important")
            def do_something_important(self):
                """Document the important things done here"""
                # add some important code here
    '''
    def toSchemaAndAttrs(self, device, state):
        h, attrs = super(Slot, self).toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.Node
        attrs["displayType"] = "Slot"
        return h, attrs

    def toDataAndAttrs(self, value):
        return Hash(), {}

    def cast(self, other):
        return Hash()

    def __get__(self, instance, owner):
        if instance is None:
            return self

        @wraps(self.method)
        def wrapper(device):
            return self.method(device)
        wrapper.slot = partial(self.slot, weakref.ref(instance))
        return wrapper.__get__(instance, owner)

    def slot(self, weakinstance, func, device, message, args):
        instance = weakinstance()
        if instance is None:
            return

        if (self.allowedStates is not None and
                instance.state not in self.allowedStates):
            msg = 'Calling slot "{}" not allowed in state "{}"'.format(
                self.key, instance.state)
            device._ss.reply(message, msg)
            device.logger.warn(msg)

        coro = get_event_loop().run_coroutine_or_thread(func)

        @coroutine
        def wrapper():
            try:
                device._ss.reply(message, (yield from coro))
            except Exception as e:
                _, exc, tb = sys.exc_info()
                instance._onException(self, exc, tb)
                device._ss.reply(message, str(e))
        return async(wrapper())

    def _initialize(self, instance, value=None):
        return []  # nothing to initialize in a Slot

    def __call__(self, method):
        """Decorate a method to be a Slot"""
        if self.description is None:
            self.description = method.__doc__
        self.method = method
        return self


class Special(object):
    """These is the base class for special types, which neither have
    a python nor numpy equivalent"""


class Type(Descriptor, Registry):
    """A Type is a descriptor that does not contain other descriptors.

    All basic Karabo types are described by a Type.
    """

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
        self.units = basetypes.QuantityValue(
                1, unit=self.unitSymbol, metricPrefix=self.metricPrefixSymbol
            ).units
        self.dimensionality = self.units.dimensionality

    def toKaraboValue(self, data, strict=True):
        """Convert data into a KaraboValue

        in strict mode, the default, we only allow values which have the
        correct unit set or are of the correct enum. In non-strict mode,
        we simply add our unit if none is given, or convert to our enum.
        This is important for data coming from the network, as that has no
        notion about units or enums.

        Note that for cirtical applications, it is advisable to do unit
        conversions and timestamp handling by hand.
        """
        raise NotImplementedError

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

    def toDataAndAttrs(self, data):
        if not isinstance(data, basetypes.KaraboValue):
            return self.cast(data), {}
        if data.timestamp is not None:
            attrs = data.timestamp.toDict()
        else:
            attrs = {}
        return data.value, attrs

    def toSchemaAndAttrs(self, device, state):
        h, attrs = super().toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.Leaf
        attrs["valueType"] = self.hashname()
        return h, attrs

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

    def toKaraboValue(self, data, strict=True):
        if not isinstance(data, basetypes.KaraboValue):
            data = self.cast(data)
        if not strict or not self.dimensionality or isinstance(data, str):
            data = basetypes.QuantityValue(data, descriptor=self)
        elif not isinstance(data, basetypes.QuantityValue):
            raise pint.DimensionalityError("no dimension", self.dimensionality)
        if data.units != self.units:
            data = data.to(self.units)
            data.descriptor = self
        return data


class Bool(Type):
    """This describes a boolean: ``True`` or ``False``"""
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

    def toKaraboValue(self, data, strict=True):
        return basetypes.BoolValue(data, descriptor=self)


class VectorBool(NumpyVector):
    basetype = Bool
    number = 1

    @classmethod
    def toString(cls, data):
        return ",".join(str(int(i)) for i in data)

    def cast(self, other):
        if isinstance(other, list) and other and isinstance(other[0], str):
            other = [o in ('true', 'True', '1') for o in other]
        return super(VectorBool, self).cast(other)

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
        try:
            return _Byte(chr(other))
        except TypeError:
            if len(str(other)) == 1:
                return _Byte(other)
            elif isinstance(other, bytes):
                o = other.decode("ascii")
                if len(o) == 1:
                    return _Byte(o)
            raise

    def toKaraboValue(self, data, strict=True):
        if isinstance(data, bytes) and len(data) == 1:
            data = data[0]
        elif not (0 <= data < 256):
            raise ValueError(
                "Character must be bytes of length 1 or positive number < 256")
        return basetypes.QuantityValue(data, descriptor=self)


class _Byte(Special, str):
    """This represents just one byte, so that we can distinguish
    CHAR and VECTOR_CHAR."""
    hashtype = Char

    def __repr__(self):
        return "${:x}".format(ord(self))


class VectorChar(Vector):
    """A VectorChar is simply some binary data in memory. The corresponding
    Python data type is :class:`python:bytes`."""
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

    def toKaraboValue(self, data, strict=True):
        return basetypes.VectorCharValue(data, descriptor=self)


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

    def toKaraboValue(self, data, strict=True):
        if self.enum is not None:
            return Enumable.toKaraboValue(self, data, strict)
        return basetypes.StringValue(data, descriptor=self)


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
            def check(s):
                if not isinstance(s, str):
                    raise TypeError
                return s
            return StringList(check(s) for s in other)

    def toKaraboValue(self, data, strict=True):
        return basetypes.VectorStringValue(data, descriptor=self)


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

    def __init__(self, rowSchema=None, strict=True, **kwargs):
        from .schema import Configurable

        super(VectorHash, self).__init__(strict=strict, **kwargs)

        if rowSchema is None:
            # This (and the default for rowSchema) is a HACK to enable the Gui
            # again to work with table elements. That was broken after
            # introduction of this VectorHash.__init__ - which itself was added
            # for partial support of the TableElement in middlelayer_api
            # (partial means: getDevice("device_with_table_elem") does not
            #  crash anymore).
            # The problematic line in the Gui code was line 523 in schema.py:
            #   ret = Type.fromname[attrs['valueType']]()
            # which requires __init__ to work without arguments.
            self.cls = None
            return

        namespace = {}
        for k, v, a in rowSchema.hash.iterall():
            desc = Type.fromname[a["valueType"]](strict=strict, key=k, **a)
            namespace[k] = desc
        self.cls = type(self.key, (Configurable,), namespace)

    @classmethod
    def read(cls, file):
        return list(super(VectorHash, cls).read(file))

    def cast(self, other):
        ht = HashType()
        return [ht.cast(o) for o in other]

    def toKaraboValue(self, data, strict=True):
        table = [
            self.cls({k: getattr(self.cls, k).toKaraboValue(v, strict=strict)
                      for k, v in row.items()})
            for row in data]
        return basetypes.TableValue(table, descriptor=self)


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
        self.name = other.name
        self.rules = other.rules

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


class Hash(OrderedDict):
    """This is the serialization data structure of Karabo

    Every data that gets transfered over the network or saved to file
    by Karabo is in this format.

    It is mostly an extended :class:`dict`.

    The big difference to normal Python containers is the dot-access method.
    The hash has a built-in knowledge about it containing itself. Thus,
    one can access subhashes by ``hash['key.subhash']``.

    The other speciality are attributes. In Python, these can be accessed
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

        This behaves like the :meth:`~dict.items` method of Python
        :class:`dict`, except that it yields not only key and value but
        also the attributes for it. """
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
