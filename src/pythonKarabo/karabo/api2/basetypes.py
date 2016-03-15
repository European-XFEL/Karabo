"""This module contains the base classes for Karabo data types

Karabo keeps some metadata with its values. This module contains the
classes which have the metadata attached."""
from enum import Enum
from functools import wraps
import inspect
from itertools import chain
import numbers
import re

import numpy
import pint

from .enums import MetricPrefix, Unit
from .registry import Registry
from .timestamp import Timestamp


def wrap(data):
    if isinstance(data, KaraboValue):
        return data
    elif isinstance(data, bool):
        return BoolValue(data)
    elif isinstance(data, str):
        return StringValue(data)
    elif isinstance(data, bytes):
        return VectorCharValue(data)
    elif isinstance(data, list):
        return VectorStringValue(data)
    elif isinstance(data, (numbers.Number, numpy.ndarray)):
        return QuantityValue(data)
    else:
        raise TypeError('cannot wrap "{}" into Karabo type'.format(type(data)))


def wrap_function(func, timestamp=None):
    """wrap the function func to return a KaraboValue with the newest
    timestamp of its parameters"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        newest = timestamp
        for a in chain(args, kwargs.values()):
            if (isinstance(a, KaraboValue) and a.timestamp is not None and
                    (newest is None or a.timestamp > newest)):
                newest = a.timestamp
        ret = func(*args, **kwargs)
        if newest is not None:
            try:
                if isinstance(ret, tuple):
                    ret = tuple(wrap(r) for r in ret)
                    for r in ret:
                        r.timestamp = newest
                else:
                    ret = wrap(ret)
                    ret.timestamp = newest
            except TypeError:
                pass  # the wrapper didn't manage to wrap, just skip it
        return ret
    return wrapper


class KaraboValue(Registry):
    """This is the baseclass for all Karabo values"""
    __re = re.compile("__.*__|[^_].*")
    __blacklist = {"__len__", "__contains__", "__complex__", "__int__",
                   "__float__", "__index__", "__bool__", "__getattribute__",
                   "__getattr__", "__init__", "__new__", "__setattr__",
                   "__array_prepare__", "__hash__", "__str__", "__repr__",
                   "__array_wrap__", "register"}

    @classmethod
    def register(cls, name, d):
        attrs = [a for a in dir(cls)
                 if cls.__re.fullmatch(a) and a not in cls.__blacklist]

        for name in attrs:
            attr = getattr(cls, name)
            if inspect.isfunction(attr) or inspect.ismethoddescriptor(attr):
                setattr(cls, name, wrap_function(attr))


class SimpleValue(KaraboValue):
    """Base class for values which need no special treatment"""
    def __init__(self, value, *, descriptor=None, timestamp=None):
        self.value = value
        self.descriptor = descriptor
        self.timestamp = timestamp


class BoolValue(SimpleValue):
    """This conains bools.

    We cannot inherit from bool, so we need a brand-new class"""
    def __init__(self, value, *, descriptor=None, timestamp=None):
        super().__init__(bool(value), descriptor=descriptor,
                         timestamp=timestamp)

    def __eq__(self, other):
        return self.value == bool(other)

    def __bool__(self):
        return self.value


class EnumValue(SimpleValue):
    """This contains an enum.

    We can define enums in the expected parameters. This contains a value of
    them. Unfortunately, it is impossible to use the "is" operator as with
    bare enums, one has to use == instead. """
    def __init__(self, value, *, descriptor=None, timestamp=None):
        if not isinstance(value, Enum):
            raise TypeError("value must be an Enum")
        if descriptor is not None and not isinstance(value, descriptor.enum):
            raise TypeError("value is not element of enum in descriptor")
        super().__init__(value, descriptor=descriptor, timestamp=timestamp)

    def __eq__(self, other):
        if isinstance(other, EnumValue):
            return self.value is other.value
        else:
            return self.value is other


class StringlikeValue(KaraboValue):
    """This mixin class extends existing Python classes"""
    def __new__(cls, value, *, descriptor=None, timestamp=None):
        self = super().__new__(cls, value)
        self.descriptor = descriptor
        self.timestamp = timestamp
        return self

    def __eq__(self, other):
        if isinstance(other, type(self)):
            return str(self) == str(other)
        else:
            return super().__eq__(other)


class VectorCharValue(StringlikeValue, bytes):
    """A Karabo VectorChar is a Python bytes object"""


class StringValue(StringlikeValue, str):
    """A Karabo String is a Python str"""


class VectorStringValue(KaraboValue, list):
    """A Karabo VectorStringValue corresponds to a Python list.

    We should check that only strings are entered"""
    def __init__(self, value=None, *, descriptor=None, timestamp=None):
        if value is None:
            super().__init__()
        else:
            super().__init__(value)
        self.descriptor = descriptor
        self.timestamp = timestamp

    def __repr__(self):
        return "VectorString" + super().__repr__(self)


# Pint is based on the concept of a unit registry. For each unit registry,
# a new class (!) is created, and quantities are only compatible if we
# use the same class. Here we define the Karabo quantity class.

unit_registry = pint.UnitRegistry()
Quantity = unit_registry.Quantity


class QuantityValue(KaraboValue, Quantity):
    """The base class for all Karabo numerical values, including vectors.

    It has a unit (by virtue of inheriting a pint Quantity).
    Vectors are represented by numpy arrays. """

    def __new__(cls, value, unit=None, metricPrefix=MetricPrefix.NONE, *,
                descriptor=None, timestamp=None):
        # weirdly, Pint uses __new__. Dunno why, but we need to follow.
        if isinstance(unit, Unit):
            if unit is Unit.NOT_ASSIGNED:
                unit = ""
            else:
                unit = unit.name
            if metricPrefix is MetricPrefix.NONE:
                prefix = ""
            else:
                prefix = metricPrefix.name
            self = super().__new__(cls, value, (prefix + unit).lower())
        else:
            self = super().__new__(cls, value, unit)
        if (descriptor is not None and
                descriptor.dimensionality != self.dimensionality):
            raise pint.DimensionalityError(descriptor.dimensionality,
                                           self.dimensionality)
        self.descriptor = descriptor
        self.timestamp = timestamp
        return self

    @property
    def T(self):
        ret = super().T
        ret.timestamp = self.timestamp
        return ret

    @property
    def real(self):
        ret = super().real
        ret.timestamp = self.timestamp
        return ret

    @property
    def imag(self):
        ret = super().imag
        ret.timestamp = self.timestamp
        return ret

    def __getattr__(self, attr):
        ret = super().__getattr__(attr)
        if callable(ret):
            return wrap_function(ret, self.timestamp)
        return ret

    def __array_wrap__(self, obj, context=None):
        ret = super().__array_wrap__(obj, context)
        if not isinstance(ret, QuantityValue):
            return ret
        _, objs, _ = context
        newest = None
        for o in objs:
            ts = getattr(o, "timestamp", None)
            if isinstance(ts, Timestamp) and (newest is None or ts > newest):
                newest = ts
        ret.timestamp = newest
        return ret


# Whenever Pint does calculations, it returns the results as an objecti
# of the registries' Quantity class. We set that to our own class so
# that we keep our data.
unit_registry.Quantity = QuantityValue

# define the Karabo units that Pint doen't know about
unit_registry.define("number = count = #")
unit_registry.define("electronvolt = eV")
unit_registry.define("degree_celsius = degC")
unit_registry.define("katal = mol / s = kat")
unit_registry.define("pixel = count = px")
unit_registry.define("meter_per_second = m / s")
unit_registry.define("volt_per_second = V / s")
unit_registry.define("ampere_per_second = A / s")
unit_registry.define("percent = count / 100 = %")
