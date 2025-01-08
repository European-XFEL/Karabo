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
"""This module contains the base classes for Karabo schema types

Karabo keeps some metadata with its values. This module contains the
classes which have the metadata attached."""
import inspect
import numbers
import operator
import re
from enum import Enum
from functools import wraps
from itertools import chain
from xml.sax.saxutils import escape

import numpy
import pint
import tabulate

from karabo.native.data import (
    EncodingType, Hash, HashList, HashType, MetricPrefix, Unit)

from ..weak import Weak

__all__ = ['BoolValue', 'EnumValue', 'ImageData', 'isSet', 'isStringSet',
           'KaraboValue', 'NoneValue', 'newest_timestamp', 'Quantity',
           'QuantityValue', 'StringValue', 'StringlikeValue', 'TableValue',
           'unit_registry', 'VectorCharValue', 'VectorStringValue', 'wrap',
           'wrap_function', 'wrap_methods']


def wrap(data):
    """Return a `KaraboValue` related to `data`"""
    if isinstance(data, KaraboValue):
        return data
    elif isinstance(data, bool):
        return BoolValue(data)
    elif isinstance(data, str):
        return StringValue(data)
    elif isinstance(data, Enum):
        return EnumValue(data)
    elif isinstance(data, (bytes, bytearray)):
        return VectorCharValue(data)
    elif isinstance(data, list):
        return VectorStringValue(data)
    elif isinstance(data, (numbers.Number, numpy.ndarray)):
        return QuantityValue(data)
    else:
        raise TypeError(f'cannot wrap "{type(data)}" into Karabo type')


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
    ignorelist = {"__len__", "__contains__", "__complex__", "__int__",
                  "__float__", "__index__", "__bool__", "__getattribute__",
                  "__getattr__", "__init__", "__new__", "__setattr__",
                  "__array_prepare__", "__hash__", "__str__", "__repr__",
                  "__array_wrap__", "register", "__array_ufunc__",
                  "__array__", "m_as"}

    attrs = [a for a in dir(cls)
             if pattern.fullmatch(a) and a not in ignorelist]

    for name in attrs:
        attr = getattr(cls, name)
        if inspect.isfunction(attr) or inspect.ismethoddescriptor(attr):
            setattr(cls, name, wrap_function(attr))
    return cls


class KaraboValue:
    """This is the base class for all Karabo values.

    All attributes of a Karabo device contain objects of these types, as they
    are the only ones we know how to transport over the network.

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

    .. attribute:: tid

      This is the macro id number attached to the timestamp.

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

    @property
    def has_tid(self):
        """Property to indicate whether our KaraboValue has a valid trainId
        """
        return self.timestamp.tid > 0

    @property
    def tid(self):
        """Property to directly provide the trainId of the KaraboValue
        """
        return self.timestamp.tid

    def __iter__(self):
        for a in super().__iter__():
            y = wrap(a)
            y.timestamp = self.timestamp
            yield y

    def _repr_html_generator_(self):
        yield escape(str(self))

    def _repr_html_(self):
        return "".join(self._repr_html_generator_())


class _Singleton(KaraboValue):
    """Base class for True, False, None"""

    def __bool__(self):
        return bool(self.value)

    def __repr__(self):
        return repr(self.value)

    def __hash__(self):
        return hash(self.value)

    def __iter__(self):
        raise TypeError(f"A singleton class like {self.__class__.__name__} "
                        f"is not iterable.")


@wrap_methods
class BoolValue(_Singleton):
    """This contains bools.

    Objects of this class behave effectively like normal bools, just
    with a timestamp and a descriptor added.
    """
    _hashType = HashType.Bool

    # We cannot inherit from bool, so we need a brand-new class

    def __init__(self, value, **kwargs):
        super().__init__(value, **kwargs)
        self.value = bool(value)

    def __int__(self):
        return int(self.value)

    def __float__(self):
        return float(self.value)

    def __eq__(self, other):
        return self.value == bool(other)


class NoneValue(_Singleton):
    """This represents a value which is not set.

    This is mostly the Karabo equivalent of `None`.
    """
    _hashType = HashType.None_
    value = None

    def __init__(self, value=None, **kwargs):
        assert value is None or isinstance(value, NoneValue)
        super().__init__(value, **kwargs)

    def __eq__(self, other):
        return other is None or isinstance(other, NoneValue)


def isSet(value):
    """Test whether *value* actually has a value"""
    return not (value is None or isinstance(value, NoneValue))


def isStringSet(value):
    """Test whether *value* actually has a non-empty string value"""
    return isSet(value) and not (not isinstance(value, str) or not value)


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
            raise TypeError(f'value "{value}" must be an Enum')
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

    def _repr_pretty_(self, p, cycle):
        p.text(f"<{self}>")

    def _repr_html_generator_(self):
        yield f"<i>{self}</i>"

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


# if you override __eq__, __hash__ gets set to None to avoid incorrect
# accidental inheritance. This fixes that.
del BoolValue.__hash__
del NoneValue.__hash__
del StringlikeValue.__hash__


@wrap_methods
class VectorCharValue(StringlikeValue, bytes):
    """A VectorChar is a Python :class:`bytes` object"""

    @property
    def value(self):
        return super().__bytes__()


@wrap_methods
class StringValue(StringlikeValue, str):
    """A StringValue is a Python :class:`str`."""

    @property
    def value(self):
        return str(self)


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

    def _repr_html_generator_(self):
        yield "<br />".join(escape(s) for s in self)

    def _repr_pretty_(self, p, cycle):
        with p.group(1, "[", "]"):
            if self:
                p.text(self.value[0])
            for s in self.value[1:]:
                p.text(",")
                p.breakable()
                p.text(s)

    @property
    def value(self):
        return list(self)


class ImageData(KaraboValue):
    """The Karabo ImageData is supposed to provide an encapsulated NDArray

    This KaraboValue can estimate from the input array the associated
    attributes of the `ImageData`, such as binning, encoding, etc.

    Every attribute can be provided as well on initialization of the object.

    :param binning: The binning of the image, e.g. [0, 0]
    :param encoding: The encoding of the image, e.g. EncodingType.GRAY (enum)
    :param rotation: The rotation of the image, either 0, 90, 180 or 270
    :param roiOffsets: The roiOffset, e.g. [0, 0]
    :param dimScales: Description of the dim scales
    :param dimTypes: The dimension types
    :param bitsPerPixel: The bits per pixel
    :param flipX: Image horizontal flip, either `True` or `False`
    :param flipY: Image vertical flip, either `True` or `False`

    :type binning: array with dtype uint64
    :type encoding: integer with dtype int32
    :type rotation: integer with dtype int32
    :type roiOffsets: array with dtype uint64
    :type dimScales: str
    :type dimTypes: array with dtype int32
    :type flipX: bool
    :type flipY: bool
    """

    def __init__(self, value, *args, binning=None, encoding=None,
                 rotation=None, roiOffsets=None, dimScales=None, dimTypes=None,
                 bitsPerPixel=None, flipX=False, flipY=False, **kwargs):
        super().__init__(value, *args, **kwargs)
        self.value = value
        self.dtype = value.dtype

        dims = numpy.array(value.shape, dtype=numpy.uint64)
        self.shape = dims
        self.dims = dims

        # NOTE: If the encoding is not provided, try to guess it!
        if encoding is None:
            if len(dims) == 2:
                encoding = EncodingType.GRAY
            elif len(dims) == 3:
                if dims[2] == 1:
                    encoding = EncodingType.GRAY
                elif dims[2] == 3:
                    encoding = EncodingType.RGB
                elif dims[2] == 4:
                    encoding = EncodingType.RGBA
                else:
                    encoding = EncodingType.UNDEFINED
            else:
                encoding = EncodingType.UNDEFINED

        if isinstance(encoding, Enum):
            encoding = encoding.value

        self.encoding = numpy.int32(encoding)

        self.dimScales = dimScales if dimScales else ""
        dimTypes = [] if dimTypes is None else dimTypes
        self.dimTypes = numpy.array(dimTypes, dtype=numpy.int32)
        binning = [1] * len(dims) if binning is None else binning
        self.binning = numpy.array(binning, dtype=numpy.uint64)
        roiOffsets = [0] * len(dims) if roiOffsets is None else roiOffsets
        self.roiOffsets = numpy.array(roiOffsets, dtype=numpy.uint64)
        rotation = 0 if rotation is None else rotation
        self.rotation = numpy.int32(rotation)

        # NOTE: Set the bits per pixel depending on the shape if `None`
        if bitsPerPixel is None:
            itemsize = value.itemsize
            if len(dims) == 2:
                bitsPerPixel = 8 * itemsize
            elif len(dims) == 3:
                bitsPerPixel = 8 * itemsize * self.shape[2]
            else:
                bitsPerPixel = 8

        self.bitsPerPixel = numpy.int32(bitsPerPixel)
        self.flipX = flipX
        self.flipY = flipY

    def toDict(self):
        """Provide the `attributes` of the `ImageData` in a dictionary form"""
        data = {
            'dtype': self.dtype,
            'shape': self.shape,
            'encoding': self.encoding,
            'dimScales': self.dimScales,
            'dimTypes': self.dimTypes,
            'dims': self.dims,
            'binning': self.binning,
            'roiOffsets': self.roiOffsets,
            'rotation': self.rotation,
            'bitsPerPixel': self.bitsPerPixel,
            'flipX': self.flipX,
            'flipY': self.flipY
        }

        return data


class TableValue(KaraboValue):
    """This wraps numpy structured arrays. Pint cannot deal with them."""

    def __init__(self, value, units, **kwargs):
        super().__init__(value, **kwargs)
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
                val.base is self.value
                or val.base is self.value.base) and (
                val.dtype.char == "O"):
            return TableValue(val, units, timestamp=self.timestamp)

        ret = wrap(val)
        ret.timestamp = self.timestamp
        if isinstance(ret, QuantityValue):
            unit = Unit(units[0]) if units[0] is not None else None
            # NOTE: metricPrefix has a default NONE
            metricPrefix = MetricPrefix(units[1])
            return QuantityValue(ret.value, unit=unit,
                                 metricPrefix=metricPrefix,
                                 timestamp=self.timestamp)
        return ret

    def __setitem__(self, item, value):
        if not isinstance(item, slice):
            item = slice(item, item + 1)
        if isinstance(value, Hash):
            value = [tuple(value[key] for key in self.value.dtype.names)]
        converted = numpy.array(value, dtype=self.value.dtype)
        if converted.shape == ():
            converted.shape = (1,)
        start, stop, stride = item.indices(len(self.value))
        if item.step is not None or len(converted) == (stop - start) // stride:
            newvalue = self.value.copy()
            newvalue[item] = converted
        else:
            newvalue = numpy.concatenate(
                (self.value[:start], converted, self.value[stop:]))
        self.descriptor.__set__(self._parent, newvalue)

    def __delitem__(self, item):
        self[item] = []

    def insert(self, index, value):
        """Insert a `value` into the TableValue at `index`"""
        self[index:index] = value

    def extend(self, value):
        """Extend the `TableValue` by a `value`"""
        self[len(self.value):] = value

    def append(self, value):
        """Append `value` to the `TableValue`"""
        self.extend(value)

    def pop(self, index=-1):
        """Pops a single TableValue from the table

        NOTE: This method can only be used with a descriptor!
        """
        v = self[index]
        self.value = numpy.delete(self.value, index)
        self.descriptor.__set__(self._parent, self.value)
        return v

    def columnIndex(self, field):
        return self.value.dtype.names.index(field)

    def where(self, field, value, op=operator.eq):
        """Return indexes of the table where a condition for a column is met

        :param field: The field of the row schema, e.g. string key
        :param value: The value for the condition
        :param op: Operator condition

        :returns: list of indexes
        """
        if field not in self.value.dtype.names:
            raise AttributeError(f"Specified field attribute {field} not"
                                 " present in column schema.")
        return numpy.where(op(self.value[field], value))[0]

    def where_value(self, field, value, op=operator.eq):
        """Filter the table with a condition for a column

        :param field: The field of the row schema, e.g. string key
        :param value: The value for the condition
        :param op: Instance of operator condition

        :returns: Filtered TableValue
        """
        indexes = self.where(field, value, op)
        value = self.value[indexes]
        return TableValue(value, self.units, timestamp=self.timestamp)

    def default_row(self):
        """Return a row Hash with default values of the `TableValue`

        Added in 2.16.X: This method safely returns a quick deepcopy!
        """
        return self.descriptor.default_row.deepcopy()

    def clear(self):
        """Clear the table element with a single message"""
        self.value = numpy.array([], dtype=self.value.dtype)
        self.descriptor.__set__(self._parent, self.value)

    def to_hashlist(self):
        """Convert the TableValue as `Hashlist` but lose the attributes"""
        ret = HashList(Hash({key: value for key, value in
                             zip(self.value.dtype.names, row)})
                       for row in self.value)
        return ret

    def __len__(self):
        return len(self.value)

    def __getattr__(self, attr):
        return getattr(self.value, attr)

    def __iter__(self):
        for row in self.value:
            yield TableValue(row, self.units, timestamp=self.timestamp)

    def iter_hashes(self):
        """Iterate over the table by providing the Hash elements

        This method does NOT provide a copy!

        Note: Added in Karabo 2.16.X
        """
        names = self.value.dtype.names
        for row in self.value:
            yield Hash({key: value for key, value in
                        zip(names, row)})

    def __repr__(self):
        table = [{key: value for key, value in
                 zip(self.value.dtype.names, row)}
                 for row in self.value]
        return tabulate.tabulate(table, headers="keys", tablefmt="grid")

    def _repr_html_generator_(self):
        yield "<table><tr>"
        for name in self.value.dtype.names:
            yield f"<th>{name}</th>"
        for row in self.value:
            yield "</tr><tr>"
            for col in row:
                yield "<td>"
                yield from col._repr_html_generator_()
                yield "</td>"
        yield "</tr></table>"


Quantity = pint.UnitRegistry.Quantity


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
                self = super().__new__(cls, value, descriptor.units)
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

    def __index__(self):
        if self.dimensionality:
            raise pint.DimensionalityError("index must be dimensionless")
        return self.magnitude.__index__()

    def __getattr__(self, attr):
        ret = super().__getattr__(attr)
        if callable(ret):
            return wrap_function(ret, self.timestamp)
        return ret

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        ret = super().__array_ufunc__(ufunc, method, *inputs, **kwargs)
        if ret is NotImplemented:
            raise NotImplementedError(
                f"Cannot compute the ufunc {ufunc.__name__}. It is "
                f"currently not supported by pint.")
        if not isinstance(ret, QuantityValue):
            return ret
        ret.timestamp = newest_timestamp(inputs)
        return ret

    def __array_wrap__(self, obj, context=None):
        ret = super().__array_wrap__(obj, context)
        if not isinstance(ret, QuantityValue):
            return ret
        _, objs, _ = context
        ret.timestamp = newest_timestamp(objs)
        return ret

    def __iadd__(self, other):
        if not isinstance(other, QuantityValue):
            self.timestamp = None
        return super().__iadd__(other)

    def __isub__(self, other):
        if not isinstance(other, QuantityValue):
            self.timestamp = None
        return super().__isub__(other)

    def __imul__(self, other):
        if not isinstance(other, QuantityValue):
            self.timestamp = None
        return super().__imul__(other)

    def __itruediv__(self, other):
        if not isinstance(other, QuantityValue):
            self.timestamp = None
        return super().__itruediv__(other)

    def __ifloordiv__(self, other):
        if not isinstance(other, QuantityValue):
            self.timestamp = None
        return super().__ifloordiv__(other)

    def _format(self, value, fmt=""):
        absolute = getattr(self.descriptor, "absoluteError", None)
        relative = getattr(self.descriptor, "relativeError", None)

        if relative is None:
            try:
                relative = numpy.finfo(value.dtype).resolution
            except (AttributeError, ValueError):
                pass

        if absolute is not None and self.value != 0:
            # TODO: this branch is not covered by tests
            err = abs(absolute / self.value)
            if relative is not None:
                err = max(err, relative)
        elif relative is not None:
            err = relative
        else:
            return f"{{:~{fmt}}}".format(value)

        err = 1 - int(numpy.log10(err))
        if err > 0:
            if isinstance(value.value, numpy.ndarray):
                # XXX: [1., 2.] will be printed as '[1.0 2.0]'
                _formatter = {'float_kind': '{{:.{}{}}}'.format(
                    err, fmt).format}
                formatted_value = numpy.array2string(value.value,
                                                     formatter=_formatter)
                ret = f"{formatted_value} {value.units:~}"
            else:
                # old behaviour for floats
                ret = f"{{:.{err}~{fmt}}}".format(1.0 * value)
        else:
            # XXX: the following string always return [0], regardless of the
            #  size of the initial array
            # TODO: this branch is not covered by tests
            ret = f"{{:~{fmt}}}".format(0 * value)
        return ret

    def _repr_pretty_(self, p, cycle):
        try:
            if self.descriptor.displayType.startswith("bin|"):
                fields = self.descriptor.displayType[4:].split(",")
                fields = (field.split(":") for field in fields)
                fields = ((int(bit), name) for bit, name in fields)
                first = True
                p.text("{ ")
                for bit, name in fields:
                    if self.value & (1 << bit):
                        if not first:
                            p.breakable()
                            p.text("| ")
                        first = False
                        p.text(name)
                p.text(" }")
                return
            formats = dict(hex="0x{:x}", oct="0o{:o}", bin="0b{:b}")
            p.text(formats[self.descriptor.displayType].format(self.value))
            return
        except (AttributeError, KeyError):
            # Attribute Errors if displayType is None
            # KeyError if we do not have the format
            pass
        p.text(self._format(self))

    _fmt_pattern = re.compile(r"([0-9]\.?[0-9]*)e(-?)\+?0*([0-9]+)")

    def _repr_html_generator_(self):
        try:
            if self.descriptor.displayType.startswith("bin|"):
                fields = self.descriptor.displayType[4:].split(",")
                fields = (field.split(":") for field in fields)
                fields = ((int(bit), name) for bit, name in fields)
                res = "<br/>".join(escape(name) for bit, name in fields
                                   if self.value & (1 << bit))
                yield res
                return
            formats = dict(hex="0x{:x}", oct="0o{:o}", bin="0b{:b}")
            yield formats[self.descriptor.displayType].format(self.value)
            return
        except (AttributeError, KeyError):
            # Attribute Errors if displayType is None
            # KeyError if we do not have the format
            pass
        fmt = self._format(self, "H")
        yield self._fmt_pattern.sub(r"\1 × 10<sup>\2\3</sup>", fmt)

    def __str__(self):
        try:
            if self.descriptor.displayType.startswith("bin|"):
                fields = self.descriptor.displayType[4:].split(",")
                fields = (field.split(":") for field in fields)
                fields = ((int(bit), name) for bit, name in fields)
                res = "|".join(name for bit, name in fields
                               if self.value & (1 << bit))
                return f"{{{res}}}"
            formats = dict(hex="0x{:x}", oct="0o{:o}", bin="0b{:b}")
            return formats[self.descriptor.displayType].format(self.value)
        except (AttributeError, KeyError):
            # Attribute Errors if displayType is None
            # KeyError if we do not have the format
            pass
        return self._format(self)


class KaraboUnitRegistry(pint.UnitRegistry):
    """Provide the Karabo Unit Registry

    Define the `Quantity` as our QuantityValue to keep our data.
    """
    Quantity = QuantityValue


unit_registry = KaraboUnitRegistry()

# define the Karabo units that Pint doesn't know about
unit_registry.define("number = []")
unit_registry.define("volt_per_second = V / s")
unit_registry.define("ampere_per_second = A / s")
unit_registry.define("electronvolt = electron_volt")
# This is differently defined in Pint
unit_registry.define("count = [] = #")
unit_registry.define("percent = number / 100 = %")
unit_registry.define("pixel = number = px")
# Backward compatibility
unit_registry.define("degree_celsius = degree_Celsius")

pint.set_application_registry(unit_registry)
