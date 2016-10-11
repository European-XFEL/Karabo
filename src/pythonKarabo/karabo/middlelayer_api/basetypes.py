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
from .weak import Weak


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


def newest_timestamp(objs, newest=None):
    """Return the newest timestamp of ``objs``

    This goes through all elements of the iterable ``objs`` and returns the
    newest timestamp they might have.

    :param newest: an additional timestamp to be taken into account
    """
    for a in objs:
        if (isinstance(a, KaraboValue) and a.timestamp is not None and
                (newest is None or a.timestamp > newest)):
            newest = a.timestamp
    return newest


def wrap_function(func, timestamp=None):
    """wrap the function func to return a KaraboValue with the newest
    timestamp of its parameters"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        ret = func(*args, **kwargs)
        newest = newest_timestamp(chain(args, kwargs.values()), timestamp)
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


def wrap_methods(cls):
    pattern = re.compile(r"__\w*__|[^_]\w*")
    blacklist = {"__len__", "__contains__", "__complex__", "__int__",
                 "__float__", "__index__", "__bool__", "__getattribute__",
                 "__getattr__", "__init__", "__new__", "__setattr__",
                 "__array_prepare__", "__hash__", "__str__", "__repr__",
                 "__array_wrap__", "register"}

    attrs = [a for a in dir(cls)
             if pattern.fullmatch(a) and a not in blacklist]

    for name in attrs:
        attr = getattr(cls, name)
        if inspect.isfunction(attr) or inspect.ismethoddescriptor(attr):
            setattr(cls, name, wrap_function(attr))
    return cls


class KaraboValue(object):
    """This is the base class for all Karabo values. All attributes of
    a Karabo device contain objects of these types, as they are the only
    ones we know how to transport over the network.

    A :class:`KaraboValue` contains attributes that describe the value
    further:

    .. attribute:: descriptor

      This contains all the details of the datatype we have. It
      is an object of :class:`~karabo.middlelayer.Descriptor`, look there for
      what it contains.

      The descriptor is only available when accessing the device attributes
      directly. Values calulated from a :class:`KaraboValue` lose their
      descriptor, as it does not apply to them anymore.

    .. attribute:: timestamp

      This is the time a value has been acquired by the underlying hardware
      devices. It is an object of :class:`~karabo.middlelayer.Timestamp`.

      When doing operations on :class:`KaraboValue`, the result takes the
      newest timestamp of the values operated on. So for the example
      ``2 * x + y``, the newest timestamp of ``x`` and ``y`` is taken,
      if both have a timestamp.

    .. attribute:: value

      This is the bare value, without any special Karabo things or units
      attached.
    """

    # Karabo stores the device this value belongs to here to do its magic
    _parent = Weak()

    def __init__(self, value, *args, timestamp=None, descriptor=None,
                 **kwargs):
        super().__init__(*args, **kwargs)
        if isinstance(value, KaraboValue) and timestamp is None:
            self.timestamp = value.timestamp
        else:
            self.timestamp = timestamp
        self.descriptor = descriptor

    def getdoc(self):
        """This is called by iPython/iKarabo to get the documentation

        See :func:`IPython.core.oinspect.getdoc`
        """
        return self.descriptor.description

    def __iter__(self):
        for a in super().__iter__():
            y = wrap(a)
            y.timestamp = self.timestamp
            yield y


@wrap_methods
class BoolValue(KaraboValue):
    """This contains bools.

    Objects of this class behave effectively like normal bools, just
    with a timestamp and a descriptor added.
    """

    # We cannot inherit from bool, so we need a brand-new class

    def __init__(self, value, **kwargs):
        super().__init__(value, **kwargs)
        self.value = bool(value)

    def __eq__(self, other):
        return self.value == bool(other)

    def __bool__(self):
        return self.value

    def __repr__(self):
        return repr(self.value)

    def __hash__(self):
        return hash(self.value)


@wrap_methods
class EnumValue(KaraboValue):
    """This contains an enum.

    We can define enums in the expected parameters. This contains a value of
    them. Unfortunately, it is impossible to use the ``is`` operator as with
    bare enums, one has to use ``==`` instead. """
    def __init__(self, value, *, descriptor=None, **kwargs):
        super().__init__(value, descriptor=descriptor, **kwargs)
        if isinstance(value, EnumValue):
            if descriptor is None:
                descriptor = value.descriptor
            value = value.enum
        if descriptor is not None and not isinstance(value, descriptor.enum):
            raise TypeError('value "{}" is not element of enum "{}"'.
                            format(value, descriptor.enum))
        if not isinstance(value, Enum):
            raise TypeError('value "{}" must be an Enum'.format(value))
        self.enum = value

    def __eq__(self, other):
        if isinstance(other, EnumValue):
            return self.enum is other.enum
        else:
            return self.enum is other

    def __str__(self):
        return str(self.enum)

    def __repr__(self):
        return repr(self.enum)

    def __hash__(self):
        return hash(self.enum)

    @property
    def value(self):
        return self.enum.value


class StringlikeValue(KaraboValue):
    """This mixin class extends existing Python classes"""

    def __new__(cls, value, **kwargs):
        return super().__new__(cls, value)

    def __eq__(self, other):
        if isinstance(other, StringlikeValue):
            return (super().__getitem__(slice(None)) ==
                    super(StringlikeValue, other).__getitem__(slice(None)))
        else:
            return super().__eq__(other)

    @property
    def value(self):
        return self


# if you override __eq__, __hash__ gets set to None to avoid incorrect
# accidental inheritance. This fixes that.
del StringlikeValue.__hash__


@wrap_methods
class VectorCharValue(StringlikeValue, bytes):
    """A VectorChar is a Python :class:`bytes` object"""


@wrap_methods
class StringValue(StringlikeValue, str):
    """A StringValue is a Python :class:`str`."""


@wrap_methods
class VectorStringValue(KaraboValue, list):
    """A Karabo VectorStringValue corresponds to a Python list.

    We check that only strings are entered
    """

    def __init__(self, value=None, **kwargs):
        super().__init__(value, **kwargs)
        if value is not None:
            self.extend(value)
            for s in self:
                if not isinstance(s, str):
                    raise TypeError(
                        "Vector of strings can only contain strings")

    def __repr__(self):
        return "VectorString" + super().__repr__()

    @property
    def value(self):
        return self


class TableValue(KaraboValue):
    """This wraps numpy structured arrays. Pint cannot deal with them."""
    def __init__(self, value, units, **kwargs):
        super(TableValue, self).__init__(value, **kwargs)
        self.value = value
        self.units = units

    def __getitem__(self, item):
        val = self.value[item]
        units = self.units
        if isinstance(item, str):
            units = units[item]
        elif self.value.dtype.fields is not None:
            return TableValue(val, units, timestamp=self.timestamp)

        if isinstance(val, numpy.ndarray) and (
                val.base is self.value or val.base is self.value.base) and (
                val.dtype.char == "O"):
            return TableValue(val, units, timestamp=self.timestamp)

        ret = wrap(val)
        ret.timestamp = self.timestamp
        if isinstance(ret, QuantityValue):
            return QuantityValue(ret.value, unit=units[0],
                                 metricPrefix=units[1],
                                 timestamp=self.timestamp)
        return ret

    def __len__(self):
        return len(self.value)

    def __getattr__(self, attr):
        return getattr(self.value, attr)


# Pint is based on the concept of a unit registry. For each unit registry,
# a new class (!) is created, and quantities are only compatible if we
# use the same class. Here we define the Karabo quantity class.

unit_registry = pint.UnitRegistry()
Quantity = unit_registry.Quantity


@wrap_methods
class QuantityValue(KaraboValue, Quantity):
    """The base class for all Karabo numerical values, including vectors.

    It has a unit (by virtue of inheriting a :ref:`pint <pint:tutorial>`
    Quantity). Vectors are represented by numpy arrays.

    In addition to the timestamp processing common to any :class:`KaraboValue`,
    a :class:`QuantityValue` also has a unit.
    """

    def __new__(cls, value, unit=None, metricPrefix=MetricPrefix.NONE, *,
                descriptor=None, timestamp=None):
        # weirdly, Pint uses __new__. Dunno why, but we need to follow.
        if timestamp is None and isinstance(value, KaraboValue):
            timestamp = value.timestamp
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
            if (unit is None and descriptor is not None and
                    not self.dimensionality and
                    not isinstance(value, QuantityValue)):
                # if pint didn't find a dimension in value,
                # get it from the descriptor
                self = cls(
                    value, unit=descriptor.unitSymbol,
                    metricPrefix=descriptor.metricPrefixSymbol)
        return self

    def __init__(self, value, unit=None, metricPrefix=None, **kwargs):
        super().__init__(value, **kwargs)
        if (self.descriptor is not None and
                self.descriptor.dimensionality != self.dimensionality):
            raise pint.DimensionalityError(self.descriptor.dimensionality,
                                           self.dimensionality)

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

    @property
    def value(self):
        return self.magnitude

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
        ret.timestamp = newest_timestamp(objs)
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
