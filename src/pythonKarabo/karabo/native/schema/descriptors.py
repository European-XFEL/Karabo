""" This module contains the type hierarchy implied by the Karabo hash.
"""
from asyncio import (
    coroutine, ensure_future, get_event_loop, iscoroutinefunction)
import base64
from enum import Enum
from functools import partial, wraps
import logging
import re
import sys

import numpy as np

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State
from karabo.native.karabo_hash import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, DaqPolicy,
    LeafType, MetricPrefix, NodeType, Unit)
from karabo.native.karabo_hash import (
    decodeXML, Hash, HashByte, HashList, Schema)
from karabo.native import KaraboError

from .basetypes import (
    BoolValue, EnumValue, isSet, NoneValue, KaraboValue,
    StringValue, TableValue, VectorCharValue, VectorStringValue, QuantityValue)
from .registry import Registry

__all__ = [
    'Attribute', 'Bool', 'ByteArray', 'Char',  'ComplexFloat',
    'ComplexDouble', 'Descriptor', 'Double', 'Enumable', 'Float',
    'Integer', 'Int8', 'Int16', 'Int32', 'Int64', 'NumpyVector', 'Number',
    'RegexString',  'Simple', 'Slot', 'String', 'Type', 'TypeHash',
    'TypeSchema', 'UInt8', 'UInt16', 'UInt32',  'UInt64', 'Vector',
    'VectorString', 'VectorFloat', 'VectorBool', 'VectorChar', 'VectorDouble',
    'VectorRegexString', 'VectorComplexFloat', 'VectorComplexDouble',
    'VectorHash', 'VectorInt8', 'VectorUInt8', 'VectorInt16', 'VectorUInt16',
    'VectorInt32', 'VectorUInt32', 'VectorInt64', 'VectorUInt64']


def get_instance_parent(instance):
    """Find the parent of the instance"""
    parent = instance
    while True:
        try:
            parent = next(iter(parent._parents))
        except StopIteration:
            break

    return parent


class Attribute(object):
    __slots__ = ["default", "dtype"]

    def __init__(self, default=None, dtype=None):
        self.dtype = dtype
        self.default = self.check(default)

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            return instance.__dict__.get(self, self.default)

    def __set__(self, instance, value):
        instance.__dict__[self] = self.check(value)

    def check(self, value):
        if self.dtype is None or not isSet(value):
            return value

        info = np.iinfo(self.dtype)
        if value < info.min or value > info.max:
            raise ValueError("Attribute {} not "
                             "in range of datatype".format(value))
        return self.dtype(value)


class Enumable(object):
    """The base class for all descriptors which can be an enumeration"""
    known_classes = {"State": State,
                     "AlarmCondition": AlarmCondition}

    def __init__(self, enum=None, classId=None, **kwargs):
        if classId is not None:
            enum = self.known_classes.get(classId)
        if enum is not None:
            assert issubclass(enum, Enum)
            self.enum = enum
        super().__init__(**kwargs)

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

    def toSchemaAndAttrs(self, device, state):
        schema, attrs = super(Enumable, self).toSchemaAndAttrs(device, state)
        if self.enum is not None:
            attrs["classId"] = self.enum.__name__
            if self.enum is State:
                attrs["leafType"] = LeafType.State.value
            elif self.enum is AlarmCondition:
                attrs["leafType"] = LeafType.AlarmCondition.value
            if self.options is None:
                attrs["options"] = [val.value
                                    for val in self.enum.__members__.values()]
            else:
                attrs["options"] = [opt.value for opt in self.options]

        return schema, attrs

    def check(self, data):
        # NOTE: The State.UNKNOWN is by default allowed for a state property!
        if self.enum is State and data == State.UNKNOWN:
            return
        super().check(data)

    def toKaraboValue(self, data, strict=True):
        if not strict and not isinstance(data, self.enum):
            data = self.enum(data)
        self.check(data)
        return EnumValue(data, descriptor=self)


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

    alarmHigh = Attribute()
    alarmInfo_alarmHigh = Attribute("")
    alarmNeedsAck_alarmHigh = Attribute(False)
    alarmLow = Attribute()
    alarmInfo_alarmLow = Attribute("")
    alarmNeedsAck_alarmLow = Attribute(False)
    warnHigh = Attribute()
    alarmInfo_warnHigh = Attribute("")
    alarmNeedsAck_warnHigh = Attribute(False)
    warnLow = Attribute()
    alarmInfo_warnLow = Attribute("")
    alarmNeedsAck_warnLow = Attribute(False)

    @classmethod
    def fromstring(cls, s):
        return cls.numpy(s)

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
        super().check(ret)

    def alarmCondition(self, data):
        if not isSet(data):
            return AlarmCondition.NONE
        if isinstance(data, KaraboValue):
            data = data.value
        if self.alarmLow is not None and data < self.alarmLow:
            return AlarmCondition.ALARM_LOW
        if self.alarmHigh is not None and data > self.alarmHigh:
            return AlarmCondition.ALARM_HIGH
        if self.warnLow is not None and data < self.warnLow:
            return AlarmCondition.WARN_LOW
        if self.warnHigh is not None and data > self.warnHigh:
            return AlarmCondition.WARN_HIGH
        return AlarmCondition.NONE

    def toKaraboValue(self, data, strict=True):
        if self.enum is not None:
            return Enumable.toKaraboValue(self, data, strict)
        if isinstance(data, KaraboValue):
            timestamp = data.timestamp
        else:
            timestamp = None
        if isinstance(data, str):
            data = QuantityValue(data, descriptor=self)
        if isinstance(data, QuantityValue):
            data = data.to(self.units).value
        self.check(data)
        data = QuantityValue(self.numpy(data), descriptor=self,
                             timestamp=timestamp)
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

    def toSchemaAndAttrs(self, device, state):
        schema, attrs = super().toSchemaAndAttrs(device, state)
        if self.options is not None:
            # assure options are serialized correctly
            opts = self.options
            if self.enum is not None:
                opts = [opt.value for opt in self.options]
            attrs["options"] = np.array(opts, dtype=self.numpy)
        return schema, attrs


class Integer(Simple, Enumable):
    """The base class for all integers"""

    def check(self, value):
        super().check(value)
        if self.enum is not None:
            return

        info = np.iinfo(self.numpy)
        if value < info.min or value > info.max:
            raise ValueError("value {} not in range of datatype".format(value))

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
    displayType = Attribute()
    requiredAccessLevel = Attribute()
    allowedStates = None
    tags = None
    archivePolicy = None
    classId = None

    def __init__(self, strict=True, key="(unknown key)",
                 allowedStates=None, archivePolicy=None, tags=None,
                 requiredAccessLevel=None, classId=None, **kwargs):
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
                    raise TypeError('allowedStates must contain States, '
                                    'not "{}"'.format(allowedStates))
            else:
                self.allowedStates = set((State(s) for s in allowedStates))
        if tags is not None:
            self.tags = set(tags)
            if not all((isinstance(s, str) for s in self.tags)):
                raise TypeError('tags must contain strings, not "{}"'.
                                format(tags))
        if archivePolicy is not None:
            self.archivePolicy = ArchivePolicy(archivePolicy)
        if classId is not None:
            self.classId = classId
        if requiredAccessLevel is None:
            if self.accessMode in (AccessMode.RECONFIGURABLE,
                                   AccessMode.INITONLY):
                self.requiredAccessLevel = AccessLevel.USER
            else:
                self.requiredAccessLevel = AccessLevel.OBSERVER
        else:
            if strict and not isinstance(requiredAccessLevel, AccessLevel):
                raise TypeError(
                    'requiredAccessLevel must be of type AccessLevel,'
                    ' got {} instead'.format(requiredAccessLevel))
            self.requiredAccessLevel = AccessLevel(requiredAccessLevel)

        self.__doc__ = self.description

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
        if self.tags is not None:
            attrs["tags"] = list(self.tags)
        if self.archivePolicy is not None:
            attrs["archivePolicy"] = self.archivePolicy.value
        if self.classId is not None:
            attrs["classId"] = self.classId

        return Hash(), attrs

    def __get__(self, instance, owner):
        if instance is None:
            return self
        return instance._getValue(self.key)

    def __set__(self, instance, value):
        if (self.assignment is Assignment.OPTIONAL and
                not isSet(value)):
            value = NoneValue(value, descriptor=self)
        else:
            value = self.toKaraboValue(value)
        value._parent = instance
        instance.setValue(self, value)

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
        if value is None:
            value = NoneValue(descriptor=self)
        else:
            value = self.toKaraboValue(value, strict=False)

        ret = self.initialize(instance, value)
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
        elif self.allowedStates is not None:
            parent = get_instance_parent(instance)
            if parent.state not in self.allowedStates:
                msg = 'Setting "{}" is not allowed in state "{}"'.format(
                    self.key, parent.state)
                raise KaraboError(msg, loglevel=logging.WARNING)

        return self._setter(instance, value)

    def checkedInit(self, instance, value=None):
        """Check whether it is allowed and initialize

        This checks whether it is allowed to initialize the value and
        returns an iterable with coroutines which may be needed for
        delayed initialization.
        """
        # initial values for READONLY attributes are ignored
        if value is None or self.accessMode is AccessMode.READONLY:
            if self.assignment is Assignment.MANDATORY:
                raise KaraboError(
                    'assignment is mandatory for "{}"'.format(self.key))
            return self._initialize(instance, self.defaultValue)
        return self._initialize(instance, value)

    def toDataAndAttrs(self, value):
        """Split value in bare data and the attributes that go with it

        Return ``data`` in a format suitable to be put into a ``Hash``,
        and the attributes (namely: the timestamp) that should go with it.
        """
        raise NotImplementedError

    def allDescriptors(self, prefix=""):
        yield prefix + self.key, self


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
    method = None
    requiredAccessLevel = Attribute(AccessLevel.USER)

    def toSchemaAndAttrs(self, device, state):
        h, attrs = super(Slot, self).toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.Node
        # Explicitly calling super's schema sets Descriptor's displayType and
        # classId attributes, not Slot's
        attrs["displayType"] = "Slot"
        attrs["classId"] = "Slot"
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
            # device is self [configurable]
            return self.method(device)

        wrapper.slot = self.slot
        wrapper.descriptor = self
        if iscoroutinefunction(self.method):
            wrapper = coroutine(wrapper)
        # NOTE: wraps loses the coroutine declaration in case async def is used
        return wrapper.__get__(instance, owner)

    def slot(self, func, device, name, message, args):
        msg = device._checkLocked(message)
        if msg is not None:
            device._ss.reply(message, msg, error=True)
            return

        func = device
        for n in name.split("."):
            func = getattr(func, n)

        coro = get_event_loop().run_coroutine_or_thread(func)

        @coroutine
        def wrapper():
            try:
                if (self.allowedStates is not None and
                        device.state not in self.allowedStates):
                    msg = ('Calling slot "{}" not allowed in '
                           'state "{}"'.format(self.key, device.state))
                    device._ss.reply(message, msg, error=True)
                    device.logger.warning(msg)
                    return
                device.lastCommand = self.method.__name__
                ret = yield from coro
                device.update()
                device._ss.reply(message, ret)
            except Exception as e:
                _, exc, tb = sys.exc_info()
                yield from device._onException(self, exc, tb)
                device._ss.replyException(message, e)

        return ensure_future(wrapper())

    def _initialize(self, instance, value=None):
        return []  # nothing to initialize in a Slot

    def __call__(self, method):
        """Decorate a method to be a Slot"""
        if self.description is None:
            self.description = self.__doc__ = method.__doc__
        self.method = method
        return self


class Type(Descriptor, Registry):
    """A Type is a descriptor that does not contain other descriptors.

    All basic Karabo types are described by a Type.
    """
    unitSymbol = Attribute(Unit.NUMBER)
    metricPrefixSymbol = Attribute(MetricPrefix.NONE)
    daqPolicy = Attribute(DaqPolicy.UNSPECIFIED)
    enum = None

    # Note: Require lookup dicts!
    types = [None] * 51
    fromname = {}
    strs = {}

    options = Attribute()

    def __init__(self, strict=True, **kwargs):
        super().__init__(strict=strict, **kwargs)
        self.units = QuantityValue(
            1, unit=self.unitSymbol, metricPrefix=self.metricPrefixSymbol
        ).units
        self.dimensionality = self.units.dimensionality

        # re-write the options. We first delete them so we don't check
        # against ourselves while generating the options
        options = self.options
        self.options = None
        if options is not None:
            if self.enum is None:
                self.options = [self.toKaraboValue(o, strict=False).value
                                for o in options]
            else:
                self.options = [self.toKaraboValue(o, strict=strict).enum
                                for o in options]

    def toKaraboValue(self, data, strict=True):
        """Convert data into a KaraboValue

        in strict mode, the default, we only allow values which have the
        correct unit set or are of the correct enum. In non-strict mode,
        we simply add our unit if none is given, or convert to our enum.
        This is important for data coming from the network, as that has no
        notion about units or enums.

        Note that for critical applications, it is advisable to do unit
        conversions and timestamp handling by hand.
        """
        raise NotImplementedError

    def check(self, ret):
        if self.options is not None and ret not in self.options:
            raise ValueError("value {} not in options {}"
                             .format(ret, self.options))

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
            cls.strs[np.dtype(cls.numpy).str] = cls

    def toDataAndAttrs(self, data):
        if not isinstance(data, KaraboValue):
            return self.cast(data), {}
        if data.timestamp is not None:
            attrs = data.timestamp.toDict()
        else:
            attrs = {}
        return data.value, attrs

    def alarmCondition(self, data):
        """return the alarm condition for given *data*"""
        return AlarmCondition.NONE

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
    """This is the base class for all vectors of data"""
    minSize = Attribute(dtype=np.uint32)
    maxSize = Attribute(dtype=np.uint32)

    @classmethod
    def register(cls, name, dict):
        super(Vector, cls).register(name, dict)
        if "basetype" in dict:
            cls.basetype.vectortype = cls

    def check(self, ret):
        if self.minSize is not None and len(ret) < self.minSize:
            raise ValueError("Vector {} of {} with size {} is shorter than "
                             "the allowed size of {}".format(
                                 ret, self.key, len(ret), self.minSize))
        if self.maxSize is not None and len(ret) > self.maxSize:
            raise ValueError("Vector {} of {} with size {} is larger than "
                             "the allowed size of {}".format(
                                 ret, self.key, len(ret), self.maxSize))
        super().check(ret)


class NumpyVector(Vector):
    """The base class for all vectors which can be represented as numpy
    vectors"""
    vstrs = {}
    numpy = np.object_

    @classmethod
    def register(cls, name, dict):
        super(NumpyVector, cls).register(name, dict)
        cls.vstrs[cls.basetype.numpy().dtype.str] = cls

    @classmethod
    def fromstring(cls, s):
        if s:
            return np.fromstring(s, sep=",", dtype=cls.basetype.numpy)
        else:
            return np.array([], dtype=cls.basetype.numpy)

    def cast(self, other):
        if (isinstance(other, np.ndarray) and
                other.dtype == self.basetype.numpy):
            ret = other
        else:
            ret = np.array(other, dtype=self.basetype.numpy)
        assert ret.ndim == 1, "can only treat one-dimensional vectors"

        self.check(ret)
        return ret

    def toKaraboValue(self, data, strict=True):
        if not isinstance(data, KaraboValue):
            data = self.cast(data)
        data = QuantityValue(data, descriptor=self)
        if data.units != self.units:
            data = data.to(self.units)
            data.descriptor = self
        self.check(data)

        return data


class Bool(Type):
    """This describes a boolean: ``True`` or ``False``"""
    number = 0
    numpy = np.bool_

    @staticmethod
    def fromstring(s):
        return bool(int(s))

    def cast(self, other):
        return bool(other)

    def toKaraboValue(self, data, strict=True):
        return BoolValue(data, descriptor=self)


class VectorBool(NumpyVector):
    basetype = Bool
    number = 1

    def cast(self, other):
        if isinstance(other, list) and other and isinstance(other[0], str):
            other = [o in ('true', 'True', '1') for o in other]
        return super(VectorBool, self).cast(other)


class Char(Simple, Type):
    number = 2
    numpy = np.uint8  # actually not used, for convenience only

    @classmethod
    def fromstring(cls, s):
        return s

    def cast(self, other):
        try:
            return HashByte(chr(other))
        except TypeError:
            if len(str(other)) == 1:
                return HashByte(other)
            elif isinstance(other, bytes):
                o = other.decode("ascii")
                if len(o) == 1:
                    return HashByte(o)
            raise

    def toKaraboValue(self, data, strict=True):
        if isinstance(data, bytes) and len(data) == 1:
            data = data[0]
        elif isinstance(data, str) and len(data) == 1:
            data = ord(data)
        elif not (0 <= data < 256):
            raise ValueError(
                "Character must be bytes or string of length 1 "
                "or positive number < 256")
        return QuantityValue(data, descriptor=self)


class VectorChar(Vector):
    """A VectorChar is simply some binary data in memory. The corresponding
    Python data type is :class:`python:bytes`."""
    basetype = Char
    number = 3
    numpy = np.object_

    @classmethod
    def fromstring(cls, s):
        return base64.b64decode(s)

    def cast(self, other):
        if isinstance(other, bytes):
            return other
        else:
            return bytes(other)

    def toKaraboValue(self, data, strict=True):
        self.check(data)
        return VectorCharValue(data, descriptor=self)


class ByteArray(Vector):
    """A ByteArray is simply some binary data in memory. The corresponding
    Python data type is :class:`python:bytearray`."""
    basetype = Char
    number = 37
    numpy = np.object_

    @classmethod
    def fromstring(cls, s):
        # XXX: should return bytearray(base64.b64decode(s))
        return base64.b64decode(s)

    def cast(self, other):
        if isinstance(other, bytearray):
            return other
        else:
            return bytearray(other)

    def toKaraboValue(self, data, strict=True):
        return VectorCharValue(data, descriptor=self)


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
    numpy = np.int16


class VectorInt16(NumpyVector):
    basetype = Int16
    number = 9


class UInt16(Integer, Type):
    number = 10
    numpy = np.uint16


class VectorUInt16(NumpyVector):
    basetype = UInt16
    number = 11


class Int32(Integer, Type):
    number = 12
    numpy = np.int32


class VectorInt32(NumpyVector):
    basetype = Int32
    number = 13


class UInt32(Integer, Type):
    number = 14
    numpy = np.uint32


class VectorUInt32(NumpyVector):
    basetype = UInt32
    number = 15


class Int64(Integer, Type):
    number = 16
    numpy = np.int64


class VectorInt64(NumpyVector):
    basetype = Int64
    number = 17


class UInt64(Integer, Type):
    number = 18
    numpy = np.uint64


class VectorUInt64(NumpyVector):
    basetype = UInt64
    number = 19


class Float(Number, Type):
    number = 20
    numpy = np.float32


class VectorFloat(NumpyVector):
    basetype = Float
    number = 21


class Double(Number, Type):
    number = 22
    numpy = np.float64


class VectorDouble(NumpyVector):
    basetype = Double
    number = 23


class ComplexFloat(Number, Type):
    number = 24
    numpy = np.complex64

    @classmethod
    def fromstring(cls, s):
        return complex(*[float(n) for n in s[1:-1].split(',')])


class VectorComplexFloat(NumpyVector):
    basetype = ComplexFloat
    number = 25


class ComplexDouble(Number, Type):
    number = 26
    numpy = np.complex128

    @classmethod
    def fromstring(cls, s):
        return complex(*[float(n) for n in s[1:-1].split(',')])


class VectorComplexDouble(NumpyVector):
    basetype = ComplexDouble
    number = 27


class String(Enumable, Type):
    """This is the type corresponding to unicode strings, which are
    supposed to be used for all human-readable strings, so for
    everything except binary data."""
    number = 28
    numpy = np.object_  # strings better be stored as objects in numpy tables

    @staticmethod
    def fromstring(s):
        return str(s)

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
        self.check(data)
        return StringValue(data, descriptor=self)


class RegexString(String):
    """The `RegexString` descriptor is used as follows::

        data = RegexString(regex="0|1")

        A corresponding displayType is automatically set.
        The descriptor validates value input on set.
    """
    classId = "RegexString"
    regex = Attribute("")

    def __init__(self, flags=0, **kwargs):
        super().__init__(**kwargs)
        self._pattern = re.compile(self.regex, flags)
        self.displayType = "RegexString"

    def initialize(self, instance, value):
        self.check(value)
        return self.setter(instance, value)

    def check(self, data):
        if not self._pattern.match(data):
            raise KaraboError(f"Value {data} does not comply with regex "
                              f"pattern {self.regex}!")
        super().check(data)


class VectorString(Vector):
    basetype = String
    number = 29
    numpy = np.object_

    # NOTE: Vectorstring should be represented as python lists
    # the np.object is simply for the table element

    @staticmethod
    def fromstring(s):
        if not s:
            return []
        return [ss.strip() for ss in s.split(',')]

    def cast(self, other):
        def check(s):
            if not isinstance(s, str):
                raise TypeError
            return s

        self.check(other)
        return [check(s) for s in other]

    def toKaraboValue(self, data, strict=True):
        self.check(data)
        return VectorStringValue(data, descriptor=self)


class VectorRegexString(VectorString):
    """The `VectorRegexString` descriptor is used as follows::

        data = VectorRegexString(regex="0|1")

        A corresponding displayType is automatically set.
        The descriptor validates value input on set.

        The regex is validated with each single value of the
        vector (list).
    """
    classId = "VectorRegexString"
    regex = Attribute("")

    def __init__(self, flags=0, **kwargs):
        super().__init__(**kwargs)
        self._pattern = re.compile(self.regex, flags)
        self.displayType = "VectorRegexString"

    def initialize(self, instance, value):
        self.check(value)
        return self.setter(instance, value)

    def check(self, data):
        super().check(data)
        for index, string in enumerate(data):
            if not self._pattern.match(string):
                raise KaraboError(f"Value {string} on index {index} does not "
                                  f"comply with regex pattern {self.regex}!")


class None_(Type):
    number = 35

    def cast(self, other):
        if other is not None:
            raise TypeError('cannot cast to None (was {})'.format(other))


class TypeHash(Type):
    number = 30

    def cast(self, other):
        if isinstance(other, Hash):
            return other
        else:
            raise TypeError('cannot cast anything to Hash (was {})'.
                            format(type(other).__name__))

    @classmethod
    def hashname(cls):
        return "HASH"


class TypeSchema(TypeHash):
    number = 32

    def cast(self, other):
        if isinstance(other, Schema):
            return other
        else:
            raise TypeError('cannot cast anything to Schema (was {})'.
                            format(type(other).__name__))

    @classmethod
    def fromstring(cls, s):
        name, xml = s.split(":", 1)
        return Schema(name, hash=decodeXML(xml))

    @classmethod
    def hashname(cls):
        return "SCHEMA"


class VectorHash(Vector):
    """A VectorHash is a table

    The VectorHash value setting can be for example done via:

    - list of tuples: [(value1, value2), ...]
    - list of Hashes: [Hash('key1', value1, 'key2', value2), ...]

    :param row: The structure of each row. This is a :class:`Configurable`
    class.
    """
    basetype = TypeHash
    number = 31
    displayType = Attribute("Table")
    rowSchema = Attribute()

    def __init__(self, rows=None, strict=True, **kwargs):
        super(VectorHash, self).__init__(strict=strict, **kwargs)
        if rows is not None:
            self.rowSchema = rows.getClassSchema()

        if self.rowSchema is None:
            raise KaraboError("The table element {} does not have row "
                              "schema".format(self.key))

        self.dtype = np.dtype([(k, Type.fromname[a["valueType"]].numpy)
                               for k, v, a in self.rowSchema.hash.iterall()])
        self.coltypes = {k: Type.fromname[a["valueType"]](strict=False, **a)
                         for k, v, a in self.rowSchema.hash.iterall()}
        self.units = {k: (a.get("unitSymbol", None),
                          a.get("metricPrefixSymbol", MetricPrefix.NONE))
                      for k, _, a in self.rowSchema.hash.iterall()}

    def cast(self, other):
        ht = TypeHash()
        return HashList(ht.cast(o) for o in other)

    def toKaraboValue(self, data, strict=True):
        timestamp = None
        if strict:
            if isinstance(data, KaraboValue):
                timestamp = data.timestamp
                data = data.value
            elif isinstance(data, list) and data:
                # NOTE: We assume a list of Hashes here!
                if isinstance(data[0], Hash):
                    data = [tuple(ele[name] for name in self.dtype.names)
                            for ele in data]
            table = np.array(data, dtype=self.dtype)
        else:
            table = []
            for datarow in data:
                tablerow = ()
                for name in self.dtype.names:
                    desc = self.coltypes[name]
                    # NOTE: This is in principle not an MDL problem, but a GUI
                    # one who lost all its casting since a year for the table
                    # element. We protect here for the time being.
                    value = desc.cast(datarow[name])
                    kvalue = desc.toKaraboValue(value, strict=False)
                    tablerow += (kvalue.value,)
                table.append(tablerow)
            table = np.array(table, dtype=self.dtype)
        return TableValue(table, descriptor=self, units=self.units,
                          timestamp=timestamp)

    def toDataAndAttrs(self, value):
        if (isinstance(value, KaraboValue)
                and value.timestamp is not None):
            attrs = value.timestamp.toDict()
        else:
            attrs = {}

        data = [Hash((col, row[col]) for col in self.dtype.names)
                for row in value.value]
        return data, attrs