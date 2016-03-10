"""This module contains the base classes for Karabo data types

Karabo keeps some metadata with its values. This module contains the
classes which have the metadata attached."""
from enum import Enum
import numbers

import numpy
import pint

from .enums import MetricPrefix, Unit


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


class KaraboValue:
    """This is the baseclass for all Karabo values"""
    pass


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


class OverloadValue(KaraboValue):
    """This mixin class extends existing Python classes"""
    def __new__(cls, value, *, descriptor=None, timestamp=None):
        self = super().__new__(cls, value)
        self.descriptor = descriptor
        self.timestamp = timestamp
        return self


class VectorCharValue(OverloadValue, bytes):
    """A Karabo VectorChar is a Python bytes object"""
    pass


class StringValue(OverloadValue, str):
    """A Karabo String is a Python str"""
    pass


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
