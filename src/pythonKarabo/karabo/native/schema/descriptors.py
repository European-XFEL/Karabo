# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
""" This module contains the type hierarchy implied by the Karabo hash.
"""
import logging
import numbers
import re
import sys
import warnings
from asyncio import get_event_loop, iscoroutinefunction
from enum import Enum
from functools import partial, wraps

import numpy as np

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State
from karabo.native import KaraboError
from karabo.native.data import (
    AccessLevel, AccessMode, ArchivePolicy, Assignment, Hash, HashByte,
    HashList, MetricPrefix, NodeType, Schema, Unit, hashtype_from_string)

from .basetypes import (
    BoolValue, EnumValue, KaraboValue, NoneValue, QuantityValue, StringValue,
    TableValue, VectorCharValue, VectorStringValue, isSet)
from .utils import get_default_value, sanitize_table_schema


def get_instance_parent(instance):
    """Find the parent of the instance"""
    import warnings
    warnings.warn("This function has been deprecated, use "
                  "`instance.get_root()` instead", stacklevel=2)
    parent = instance
    while True:
        try:
            parent = next(iter(parent._parents))
        except StopIteration:
            break

    return parent


# This is a variable used to identify a `None` dtype for the `Attribute` only!
Undefined = None


class Attribute:
    """The base attribute class for Karabo descriptors

    The attribute can validate and check for a provided `dtype`.

    Note: A floating point dtype is always upgraded to have
          at least 64 bits.
    """
    __slots__ = ["default", "dtype", "name"]

    def __init__(self, default=None, dtype=None):
        if dtype is np.float32:
            dtype = np.float64
        self.name = None
        self.dtype = dtype
        self.default = self.check(default)

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            return instance.__dict__.get(self, self.default)

    def __set__(self, instance, value):
        instance.__dict__[self] = self.check(value)

    def __set_name__(self, owner, name):
        self.name = name

    def check(self, value):
        if self.dtype is None or not isSet(value):
            return value
        elif isinstance(value, Enum):
            # This can be a defaultValue on an integer descriptor or
            # the normal `Enum` Attribute values such as AccessLevel, etc.
            # If our dtype is thus an `Enum`, we have to validate!
            if issubclass(self.dtype, Enum) and value not in self.dtype:
                raise ValueError(f"Attribute {self.name} with {value} not "
                                 f"validated with Enum {self.dtype}")
            return value
        elif self.dtype is str:
            if not isinstance(value, str):
                raise ValueError("A string is required for Attribute "
                                 f"{self.name}, but got {type(value)} instead")
            return value
        elif self.dtype is bool:
            if type(value) is not bool:
                raise ValueError(f"A boolean is required for Attribute "
                                 f"{self.name}, but got {value} instead")
            return value
        elif issubclass(self.dtype, Enum):
            # Note: Unlikely case, but value must be casted to `Enum`.
            return self.dtype(value)

        if issubclass(self.dtype, np.integer):
            info = np.iinfo(self.dtype)
        else:
            info = np.finfo(self.dtype)
        if value < info.min or value > info.max:
            raise ValueError(f"Attribute {self.name} with {value} not "
                             f"in range of datatype {self.dtype}")

        return self.dtype(value)


class Enumable:
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
            raise TypeError(f"{self.enum} required here")

    def toDataAndAttrs(self, data):
        h, attrs = super().toDataAndAttrs(data)
        if self.enum is not None:
            return data.value, attrs
        else:
            return h, attrs

    def toSchemaAndAttrs(self, device, state):
        schema, attrs = super().toSchemaAndAttrs(device, state)
        if self.enum is not None:
            attrs["classId"] = self.enum.__name__
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


class Simple:
    """This is the base for all numeric types

    It features the minimum and maximum for its values given
    in ``minExc``, ``maxExc``, ``minInc``, and ``maxInc``, where
    ``min`` and ``max`` stand for miniumum and maximum, while ``Exc``
    and ``Inc`` stand for exclusive and inclusive, respectively.

    Do not use inclusive and exclusive for the same limit at the same
    time.
    """

    # For the `Simple` classes, the `dtype` to be defined by subclasses
    minExc = Attribute(dtype=Undefined)
    maxExc = Attribute(dtype=Undefined)
    minInc = Attribute(dtype=Undefined)
    maxInc = Attribute(dtype=Undefined)

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
            raise ValueError(f"value {value} not in range of datatype")

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

    def __init__(self, strict=True, **kwargs):
        if "absoluteError" in kwargs:
            kwargs.pop("absoluteError")
            warnings.warn(
                "DeprecationWarning: `absoluteError` has been deprecated and "
                "will be removed in a future release. This warning will "
                "result in an error in the future.",
                category=DeprecationWarning,
                stacklevel=2)
        if "relativeError" in kwargs:
            kwargs.pop("relativeError")
            warnings.warn(
                "DeprecationWarning: `relativeError` has been deprecated and "
                "will be removed in a future release. This warning will "
                "result in an error in the future.",
                category=DeprecationWarning,
                stacklevel=2)

        super().__init__(strict=strict, **kwargs)

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


class Descriptor:
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

    displayedName = Attribute(dtype=str)
    alias = Attribute(dtype=Undefined)
    description = Attribute(dtype=str)
    defaultValue = Attribute(dtype=Undefined)
    accessMode = Attribute(AccessMode.RECONFIGURABLE, dtype=AccessMode)
    assignment = Attribute(Assignment.OPTIONAL, dtype=Assignment)
    displayType = Attribute(dtype=str)
    requiredAccessLevel = Attribute(dtype=AccessLevel)
    archivePolicy = Attribute(dtype=ArchivePolicy)
    allowedStates = None
    tags = None
    classId = None

    def __init__(self, strict=True, key="(unknown key)",
                 allowedStates=None, tags=None, requiredAccessLevel=None,
                 classId=None, **kwargs):
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
                    if strict and (converted != v or isinstance(
                            converted, Enum) and not isinstance(v, Enum)):
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
                if not all(isinstance(s, State) for s in self.allowedStates):
                    raise TypeError('allowedStates must contain States, '
                                    'not "{}"'.format(allowedStates))
            else:
                self.allowedStates = {State(s) for s in allowedStates}
        if tags is not None:
            self.tags = set(tags)
            if not all(isinstance(s, str) for s in self.tags):
                raise TypeError('tags must contain strings, not "{}"'.
                                format(tags))
        if classId is not None:
            self.classId = classId
        if requiredAccessLevel is None:
            if self.accessMode in (AccessMode.RECONFIGURABLE,
                                   AccessMode.INITONLY):
                self.requiredAccessLevel = AccessLevel.OPERATOR
            else:
                self.requiredAccessLevel = AccessLevel.OBSERVER
        else:
            if strict and not isinstance(requiredAccessLevel, AccessLevel):
                raise TypeError(
                    'requiredAccessLevel must be of type AccessLevel,'
                    ' got {} instead'.format(requiredAccessLevel))
            self.requiredAccessLevel = AccessLevel(requiredAccessLevel)

        self.__doc__ = self.description

    def __set_name__(self, owner, name):
        self.key = name

    @property
    def attributes(self):
        """Return the attributes of this descriptor in a dictionary form

        This method preserves the `Enum` values of the common attributes.
        """
        attrs = ((name, getattr(self, name)) for name in dir(type(self))
                 if isinstance(getattr(type(self), name), Attribute))
        attrs = {name: attr for name, attr in attrs if attr is not None}
        if self.allowedStates is not None:
            attrs["allowedStates"] = [s.value for s in self.allowedStates]
        if self.tags is not None:
            attrs["tags"] = list(self.tags)
        if self.archivePolicy is not None:
            attrs["archivePolicy"] = self.archivePolicy
        if self.classId is not None:
            attrs["classId"] = self.classId

        return attrs

    def toSchemaAndAttrs(self, device, state):
        """return schema for device in state

        This returns the Hash representing this descriptor in a Schema, as well
        as the attributes that have to be set for it.

        if device is None, the class' Schema is returned, otherwise the
        device's. If state is not None, only the parts of the schema available
        in the given state are returned.
        """
        attrs = self.attributes
        attrs.update((name, value.value) for name, value in attrs.items()
                     if isinstance(value, Enum))

        return Hash(), attrs

    def __get__(self, instance, owner):
        if instance is None:
            return self
        return instance._getValue(self.key)

    def __set__(self, instance, value):
        if (self.assignment is not Assignment.MANDATORY and
                not isSet(value)):
            # Assignment.INTERNAL and Assignment.OPTIONAL
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
            msg = f'property "{self.key}" is not reconfigurable'
            raise KaraboError(msg)
        elif self.allowedStates is not None:
            parent = instance.get_root()
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
                    f'assignment is mandatory for "{self.key}"')
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
    requiredAccessLevel = Attribute(AccessLevel.OPERATOR, dtype=AccessLevel)

    def toSchemaAndAttrs(self, device, state):
        h, attrs = super().toSchemaAndAttrs(device, state)
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

        # device is self [configurable]
        if iscoroutinefunction(self.method):
            @wraps(self.method)
            async def wrapper(device):
                return await self.method(device)
        else:
            @wraps(self.method)
            def wrapper(device):
                return self.method(device)

        wrapper.slot = self.slot
        wrapper.descriptor = self

        return wrapper.__get__(instance, owner)

    def slot(self, func, device, name, message, args):
        msg = device._checkLocked(message)
        if msg is not None:
            device._sigslot.reply(message, msg, error=True)
            return

        func = device
        for n in name.split("."):
            func = getattr(func, n)

        async def wrapper():
            try:
                sigslot = device._sigslot
                if len(args):
                    raise KaraboError(
                        f"Command Slot `{name}` does not take "
                        f"arguments, got: {args}")
                if (self.allowedStates is not None and
                        device.state not in self.allowedStates):
                    msg = ('Calling slot "{}" not allowed in '
                           'state "{}"'.format(self.key, device.state))
                    sigslot.reply(message, msg, error=True)
                    device.logger.warning(msg)
                    return

                caller = sigslot.get_property(message, "signalInstanceId")
                coro = get_event_loop().run_coroutine_or_thread(func)
                device.lastCommand = f"{name} <- {caller}"
                ret = await coro
                device.update()
                sigslot.reply(message, ret)
            except BaseException as e:
                # In Python 3.8 changed to BaseException
                _, exc, tb = sys.exc_info()
                await device._onException(self, exc, tb)
                sigslot.replyException(message, e)

        return get_event_loop().create_task(wrapper(), instance=device)

    def _initialize(self, instance, value=None):
        return []  # nothing to initialize in a Slot

    def __call__(self, method):
        """Decorate a method to be a Slot"""
        if self.description is None:
            self.description = self.__doc__ = method.__doc__
        self.method = method
        return self


class Type(Descriptor):
    """A Type is a descriptor that does not contain other descriptors.

    All basic Karabo types are described by a Type.
    """
    unitSymbol = Attribute(Unit.NUMBER, dtype=Unit)
    metricPrefixSymbol = Attribute(MetricPrefix.NONE, dtype=MetricPrefix)
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
            raise ValueError(f"value {ret} not in options {self.options}"
                             f" for key {self.key}")

    @classmethod
    def fromstring(cls, s):
        return hashtype_from_string(cls.number, s)

    @classmethod
    def hashname(cls):
        return cls._hashname

    @classmethod
    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        if "number" in cls.__dict__:
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
        if 'numpy' in cls.__dict__:
            cls.strs[np.dtype(cls.numpy).str] = cls

    def toDataAndAttrs(self, data):
        if not isinstance(data, KaraboValue):
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
    """This is the base class for all vectors of data"""
    minSize = Attribute(dtype=np.uint32)
    maxSize = Attribute(dtype=np.uint32)

    @classmethod
    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        if "basetype" in cls.__dict__:
            cls.basetype.vectortype = cls

    def check(self, ret):
        if self.minSize is not None and len(ret) < self.minSize:
            raise ValueError(
                "Vector {} of {} with size {} is shorter than "
                "the allowed size of {}".format(
                    ret, self.key, len(ret), self.minSize))
        if self.maxSize is not None and len(ret) > self.maxSize:
            raise ValueError(
                "Vector {} of {} with size {} is larger than "
                "the allowed size of {}".format(
                    ret, self.key, len(ret), self.maxSize))
        super().check(ret)


class NumpyVector(Vector):
    """The base class for all vectors which can be represented as numpy
    vectors"""
    vstrs = {}
    numpy = np.object_

    @classmethod
    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        cls.vstrs[cls.basetype.numpy().dtype.str] = cls

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
    defaultValue = Attribute(dtype=bool)

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
        return super().cast(other)


class Char(Simple, Type):
    number = 2
    numpy = np.uint8  # actually not used, for convenience only

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

    def cast(self, other):
        if isinstance(other, bytes):
            return other
        elif isinstance(other, str):
            return bytes(other, 'utf-8')
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

    def cast(self, other):
        if isinstance(other, bytearray):
            return other
        elif isinstance(other, str):
            return bytearray(other, 'utf-8')
        else:
            return bytearray(other)

    def toKaraboValue(self, data, strict=True):
        return VectorCharValue(data, descriptor=self)


def _create_numpy_integer(name, number, numpy):
    class NumpyInteger(Integer, Type):
        defaultValue = Attribute(dtype=numpy)

        minExc = Attribute(dtype=numpy)
        maxExc = Attribute(dtype=numpy)
        minInc = Attribute(dtype=numpy)
        maxInc = Attribute(dtype=numpy)

    return type(name, (NumpyInteger,), {"number": number,
                                        "numpy": numpy})


UInt8 = _create_numpy_integer("UInt8", 6, np.uint8)
UInt16 = _create_numpy_integer("UInt16", 10, np.uint16)
UInt32 = _create_numpy_integer("UInt32", 14, np.uint32)
UInt64 = _create_numpy_integer("UInt64", 18, np.uint64)
Int8 = _create_numpy_integer("Int8", 4, np.int8)
Int16 = _create_numpy_integer("Int16", 8, np.int16)
Int32 = _create_numpy_integer("Int32", 12, np.int32)
Int64 = _create_numpy_integer("Int64", 16, np.int64)

# Hide implementation
del _create_numpy_integer


def _create_numpy_floating(name, number, numpy):
    class NumpyFloating(Number, Type):
        defaultValue = Attribute(dtype=numpy)

        minExc = Attribute(dtype=numpy)
        maxExc = Attribute(dtype=numpy)
        minInc = Attribute(dtype=numpy)
        maxInc = Attribute(dtype=numpy)

    return type(name, (NumpyFloating,), {"number": number,
                                         "numpy": numpy})


Float = _create_numpy_floating("Float", 20, np.float32)
Double = _create_numpy_floating("Double", 22, np.float64)

# Hide implemenation
del _create_numpy_floating


def _create_numpy_vector(name, number, numpy, basetype):
    """Create a numpy vector with a specific default value"""

    class VectorDefault(Attribute):
        """Attribute class to set a defaultValue for a numpy vector"""

        def __init__(self, default=None, dtype=None):
            super().__init__(default, dtype)

        def check(self, value):
            if self.dtype is None or not isSet(value):
                return value
            if not isinstance(value, (list, np.ndarray)):
                raise KaraboError(
                    "Default value for a vector must be either "
                    f"a list or ndarray, got {type(value)} instead.")
            if isinstance(value, np.ndarray) and value.ndim != 1:
                raise KaraboError("Only 1 dimensional vectors are allowed.")

            # Vector defaults are lists...
            array = [self.dtype(v) for v in value]
            return array

    ns_dict = {"number": number, "basetype": basetype,
               "defaultValue": VectorDefault(dtype=numpy)}
    return type(name, (NumpyVector,), ns_dict)


VectorInt8 = _create_numpy_vector(
    "VectorInt8", 5, np.int8, Int8)
VectorUInt8 = _create_numpy_vector(
    "VectorUInt8", 7, np.uint8, UInt8)
VectorInt16 = _create_numpy_vector(
    "VectorInt16", 9, np.int16, Int16)
VectorUInt16 = _create_numpy_vector(
    "VectorUInt16", 11, np.uint16, UInt16)
VectorInt32 = _create_numpy_vector(
    "VectorInt32", 13, np.int32, Int32)
VectorUInt32 = _create_numpy_vector(
    "VectorUInt32", 15, np.uint32, UInt32)
VectorInt64 = _create_numpy_vector(
    "VectorInt64", 17, np.int64, Int64)
VectorUInt64 = _create_numpy_vector(
    "VectorUInt64", 19, np.uint64, UInt64)
VectorFloat = _create_numpy_vector(
    "VectorFloat", 21, np.float32, Float)
VectorDouble = _create_numpy_vector(
    "VectorDouble", 23, np.float64, Double)

del _create_numpy_vector


class String(Enumable, Type):
    """This is the type corresponding to unicode strings, which are
    supposed to be used for all human-readable strings, so for
    everything except binary data."""
    number = 28
    numpy = np.object_  # strings better be stored as objects in numpy tables
    defaultValue = Attribute(dtype=str)

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

    def check(self, data):
        if not self._pattern.match(data):
            raise KaraboError(f"Value {data} does not comply with regex "
                              f"pattern {self.regex}!")
        super().check(data)


class VectorStringDefault(Attribute):
    """Attribute class to set a defaultValue for a numpy vector"""

    def check(self, value):
        """Reimplemented method of `Attribute`"""
        if not isSet(value):
            return value
        if not isinstance(value, list):
            raise KaraboError(
                "Default value for a vector string must be a list, "
                f"got {type(value)} instead.")
        if not all([isinstance(item, str) for item in value]):
            raise ValueError(
                "VectorStrings require strings in their default value.")
        return value


class VectorString(Vector):
    basetype = String
    number = 29
    numpy = np.object_
    # NOTE: Vectorstring should be represented as python lists
    # the np.object is simply for the table element
    defaultValue = VectorStringDefault()

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

    def check(self, data):
        super().check(data)
        for index, string in enumerate(data):
            if not self._pattern.match(string):
                raise KaraboError(f"Value {string} on index {index} does not "
                                  f"comply with regex pattern {self.regex}!")


class TypeNone(Type):
    number = 35

    def cast(self, other):
        if other is not None:
            raise TypeError(f'cannot cast to None (was {other})')


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
    def hashname(cls):
        return "SCHEMA"


class VectorHash(Vector):
    """A VectorHash is a table

    The VectorHash value setting can be for example done via:

    - list of tuples: [(value1, value2), ...]
    - list of Hashes: [Hash('key1', value1, 'key2', value2), ...]

    A VectorHash can be initialized via::

        class RowSchema(Configurable):
            deviceId = String()
            classId = String()

        table = VectorHash(
            displayedName="Table",
            rows=RowSchema)

    :param row: The structure of each row. This is a :class:`Configurable`
                class.
    """
    basetype = TypeHash
    number = 31
    displayType = Attribute("Table")
    rowSchema = Attribute()

    def __init__(self, rows=None, strict=True, **kwargs):
        super().__init__(strict=strict, **kwargs)
        if rows is not None:
            self.rowSchema = rows.getClassSchema()

        if self.rowSchema is None or self.rowSchema.hash.empty():
            raise KaraboError("The table element {} does not have a valid row "
                              "schema".format(self.key))
        else:
            readonly = self.accessMode is AccessMode.READONLY
            self.rowSchema = sanitize_table_schema(self.rowSchema, readonly)

        dtype = []
        self.bindings = {}
        self.units = {}
        self.default_row = Hash()
        for k, v, a in self.rowSchema.hash.iterall():
            desc = Type.fromname[a["valueType"]](strict=False, **a)
            dtype.append((k, desc.numpy))
            self.bindings.update({k: desc})
            self.units.update({k: (a.get("unitSymbol", None),
                                   a.get("metricPrefixSymbol",
                                         MetricPrefix.NONE))})
            self.default_row.update({k: get_default_value(desc, force=True)})
        self.dtype = np.dtype(dtype)

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
            table = np.empty(len(data), dtype=self.dtype)
            for index, row_data in enumerate(data):
                row = [self.bindings[name].toKaraboValue(
                    self.bindings[name].cast(
                        row_data[name]), strict=False).value
                       for name in self.dtype.names]
                table[index] = tuple(row)

        self.check(table)
        return TableValue(table, descriptor=self, units=self.units,
                          timestamp=timestamp)

    def toDataAndAttrs(self, value):
        if (isinstance(value, KaraboValue)
                and value.timestamp is not None):
            attrs = value.timestamp.toDict()
        else:
            attrs = {}

        data = HashList(Hash((col, row[col]) for col in self.dtype.names)
                        for row in value.value)
        return data, attrs


def get_descriptor_from_data(data):
    """Derive and return an associated descriptor from `data`"""
    try:
        if data.ndim == 1 and isinstance(data, np.ndarray):
            return NumpyVector.vstrs[data.dtype.str]
        else:
            return Type.strs[data.dtype.str]
    except AttributeError:
        if isinstance(data, Hash):
            return TypeHash
        elif isinstance(data, Schema):
            return TypeSchema
        elif isinstance(data, HashList):
            return VectorHash
        elif isinstance(data, (bool, BoolValue)):
            return Bool
        elif isinstance(data, (Enum, numbers.Integral)):
            return Int32
        elif isinstance(data, numbers.Real):
            return Double
        elif isinstance(data, bytes):
            return VectorChar
        elif isinstance(data, str):
            return String
        elif isinstance(data, list):
            if data:
                return get_descriptor_from_data(data[0]).vectortype
            else:
                return VectorString
        elif not isSet(data):
            return TypeNone
        try:
            memoryview(data)
            return ByteArray
        except TypeError:
            raise TypeError(f'unknown datatype {data.__class__}')
