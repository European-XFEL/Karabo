# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from urllib.parse import parse_qs

import numpy as np
from numpy import log10
from traits.api import Undefined

from karabo.common import const
from karabo.common.api import (
    KARABO_SCHEMA_ABSOLUTE_ERROR, KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC,
    KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC)
from karabo.common.scenemodel.const import DEFAULT_DECIMALS, DEFAULT_FORMAT
from karabo.native import Hash, Timestamp, simple_deepcopy

from . import binding_types as types


def get_editor_value(property_proxy, default=None):
    """Return the correct value of a PropertyProxy to show in an editor

    :param default: return default if value is `None` or `Undefined`
    """
    value = property_proxy.edit_value
    if value is None or value is Undefined:
        return get_binding_value(property_proxy, default)
    return value


def get_binding_value(binding, default=None):
    """Get the binding value, this function is used to deal with `Undefined`
    or `None` binding values.

    :param default: return default if value is Undefined or None
    """
    value = binding.value
    return default if value is None or value is Undefined else value


def get_min_max(binding):
    """Given a BaseBinding instance, return the minimum and maximum values
    which can be assigned to its `value` trait.
    """
    INT_TYPES = (types.Int8Binding, types.Int16Binding, types.Int32Binding,
                 types.Int64Binding, types.Uint8Binding, types.Uint16Binding,
                 types.Uint32Binding, types.Uint64Binding)
    FLOAT_TYPES = (types.FloatBinding, types.ComplexBinding)

    if isinstance(binding, (INT_TYPES, FLOAT_TYPES)):
        return binding.getMinMax()

    # XXX: raise an exception?
    return None, None


def get_numpy_binding(binding):
    """Retrieve a corresponding numpy `dtype` to a binding `binding`

    Note: In case of vector types, the appropriate `dtype` of an element
    is returned.
    """
    to_numpy_dtype = {
        types.Int8Binding: np.int8,
        types.Uint8Binding: np.uint8,
        types.Int16Binding: np.int16,
        types.Uint16Binding: np.uint16,
        types.Int32Binding: np.int32,
        types.Uint32Binding: np.uint32,
        types.Int64Binding: np.int64,
        types.Uint64Binding: np.uint64,
        types.FloatBinding: np.float64,
        types.ComplexBinding: np.complex128,
        types.VectorInt8Binding: np.int8,
        types.VectorUint8Binding: np.uint8,
        types.VectorInt16Binding: np.int16,
        types.VectorUint16Binding: np.uint16,
        types.VectorInt32Binding: np.int32,
        types.VectorUint32Binding: np.uint32,
        types.VectorInt64Binding: np.int64,
        types.VectorUint64Binding: np.uint64,
        types.VectorFloatBinding: np.float32,
        types.VectorDoubleBinding: np.float64,
        types.VectorComplexFloatBinding: np.complex64,
        types.VectorComplexDoubleBinding: np.complex128
    }
    return to_numpy_dtype.get(type(binding), None)


def get_native_min_max(binding):
    """Returns the appropriate numeric minimum and maximum for a binding

    Note: In of a vector binding the numeric limits of an element are returned

    This function neglects binding specific minimum and maximum
    """
    VECTOR_UNSIGNED = (
        types.VectorUint8Binding, types.VectorUint16Binding,
        types.VectorUint32Binding, types.VectorUint64Binding)
    VECTOR_SIGNED = (
        types.VectorInt8Binding, types.VectorInt16Binding,
        types.VectorInt32Binding, types.VectorInt64Binding)
    VECTOR_INTEGER_BINDINGS = VECTOR_SIGNED + VECTOR_UNSIGNED

    if isinstance(binding, (types.IntBinding, VECTOR_INTEGER_BINDINGS)):
        numpy = get_numpy_binding(binding)
        info = np.iinfo(numpy)
        return info.min, info.max

    elif isinstance(binding, (types.FloatBinding, types.VectorFloatBinding,
                              types.VectorComplexDoubleBinding)):
        numpy = get_numpy_binding(binding)
        info = np.finfo(numpy)
        return info.min, info.max

    return None, None


def get_min_max_size(binding):
    """Given a BaseBinding instance, return the minimum and maximum size
    which can be assigned to the vector trait
    """
    if isinstance(binding, types.VectorBinding):
        min_size = binding.attributes.get(const.KARABO_SCHEMA_MIN_SIZE)
        max_size = binding.attributes.get(const.KARABO_SCHEMA_MAX_SIZE)

        return min_size, max_size

    return None, None


def has_min_max_attributes(binding):
    """Check if there is a limit pair (inclusive, exclusive) in the attributes
    """
    attrs = binding.attributes
    min_inc = attrs.get(KARABO_SCHEMA_MIN_INC)
    min_exc = attrs.get(KARABO_SCHEMA_MIN_EXC)
    max_inc = attrs.get(KARABO_SCHEMA_MAX_INC)
    max_exc = attrs.get(KARABO_SCHEMA_MAX_EXC)

    return ((min_inc is not None or min_exc is not None) and
            (max_inc is not None or max_exc is not None))


# Ensure correct mapping of reference type to numpy dtype.
REFERENCE_TYPENUM_TO_DTYPE = {
    0: 'bool',
    2: 'char',
    4: 'int8',
    6: 'uint8',
    8: 'int16',
    10: 'uint16',
    12: 'int32',
    14: 'uint32',
    16: 'int64',
    18: 'uint64',
    20: 'float32',
    22: 'float64',
}


def get_binding_array_value(binding, default=None):
    """Retrieve the array and timestamp data from a `binding` belonging
    to an array binding

    :param default: default value to be returned if no value is available

    This function checks for `Undefined` and `None` data.
    If not data is available the `default` is returned with actual timestamp.

    :returns: data, timestamp
    """
    if binding.__class__.__name__.startswith('Vector'):
        value = binding.value
        if value is None or value is Undefined:
            return default, Timestamp()

        return value, binding.timestamp

    # We now have an `NDArray`
    node = binding.value
    if node is Undefined:
        return default, Timestamp()

    pixels = node.data.value
    if pixels is None:
        return default, Timestamp()

    arr_type = REFERENCE_TYPENUM_TO_DTYPE.get(node.type.value)
    if arr_type is None:
        # Unknown or not supported data type
        return default, Timestamp()

    value = np.frombuffer(pixels, dtype=arr_type)
    timestamp = node.data.timestamp
    # Note: Current traits always casts to 1dim
    if value.ndim == 1:
        return value, timestamp

    return default, Timestamp()


def attr_fast_deepcopy(d):
    """copy.deepcopy is criminally slow. We can bypass its fanciness as long
    as we only copy 'simple' datastructures."""
    return {k: simple_deepcopy(v) for k, v in d.items()}


def realign_hash(hsh, reference):
    """Reorders and fills the Hash `hsh` if necessary with respect to the
    key order `sequence`.

    - The Hash is build up according to `reference` and filled (`None`) if
     if the respective key is not in `hsh`.
    - Afterwards, if a key of the Hash `hsh` is not in the reference, it is
    appended at the end and assigned a `None` value.

    :param hsh: The Hash object
    :param reference: a sequence (list) of string keys as reference

    :return (Hash): new realigned `Hash`
    """

    ret = Hash()
    # 1. First iterate over the reference list to build up the new Hash
    for old_key in reference:
        if old_key in hsh:
            value, attrs = hsh.getElement(old_key)
            ret.setElement(old_key, simple_deepcopy(value),
                           simple_deepcopy(attrs))
        else:
            ret.setElement(old_key, None, {})

    # 2. Then, refill the hash keys that might not be in reference and append
    for key, value, attrs in Hash.flat_iterall(hsh):
        if key in reference:
            # Already considered
            continue

        ret.setElement(key, simple_deepcopy(value), simple_deepcopy(attrs))

    return ret


def get_dtype_format(binding):
    """Get a display type formatter for the binding

    :param binding: `binding` type instance with display type
    """
    fmt = "{}"
    if isinstance(binding, types.IntBinding):
        dt = binding.display_type.split("|")
        prefix = dt[0]
        _fmt = {
            "bin": "0b{:b}",
            "oct": "o{:o}",
            "hex": "0x{:X}"}
        try:
            fmt = _fmt[prefix]
        except (TypeError, KeyError):
            pass
    elif isinstance(binding, types.FloatBinding):
        fmt = "{:.8g}"
        abs_err = binding.attributes.get(KARABO_SCHEMA_ABSOLUTE_ERROR)
        if abs_err is not None:
            # Yes, abs error below 0 can happen
            if 0 < abs_err < 1:
                fmt = f"{{:.{-int(log10(abs_err))}f}}"
            else:
                fmt = "{:.1f}"

    return fmt


def get_binding_format(binding):
    """Read the initial format information from a binding"""
    display = binding.display_type.split("|")
    if display[0] != "format" or len(display) != 2:
        return DEFAULT_FORMAT, DEFAULT_DECIMALS

    parsed = parse_qs(display[1], keep_blank_values=False)
    attrs = {name: opt[0] for name, opt in parsed.items()}

    fmt = attrs.get("fmt", DEFAULT_FORMAT)
    frac = attrs.get("decimals", DEFAULT_DECIMALS)

    return fmt, frac


# Binding class utils
# ---------------------------------------------------------------------------

_SIGNED_VECTOR_INTEGER = (
    types.VectorInt8Binding,
    types.VectorInt16Binding,
    types.VectorInt32Binding,
    types.VectorInt64Binding)

_UNSIGNED_VECTOR_INTEGER = (
    types.VectorUint8Binding,
    types.VectorUint16Binding,
    types.VectorUint32Binding,
    types.VectorUint64Binding)


def is_signed_vector_integer(binding):
    """Return if the `binding` is a signed vector int binding"""
    return isinstance(binding, _SIGNED_VECTOR_INTEGER)


def is_unsigned_vector_integer(binding):
    """Return if the `binding` is an unsigned vector int binding"""
    return isinstance(binding, _UNSIGNED_VECTOR_INTEGER)


def is_vector_integer(binding):
    """Return if the `binding` belongs to vector int binding"""
    return (is_unsigned_vector_integer(binding) or
            is_signed_vector_integer(binding))


def is_vector_floating(binding):
    return isinstance(binding, (types.VectorFloatBinding,
                                types.VectorDoubleBinding))
