import numpy as np
from traits.api import Undefined

from karabo.common import const
from karabo.common.api import (
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC)
from . import types


def get_editor_value(property_proxy, default=None):
    """Return the correct value of a PropertyProxy to show in an editor

    :param default: return default if value is None or Undefined
    """
    value = property_proxy.edit_value
    if value is None or value is Undefined:
        return get_binding_value(property_proxy, default)
    return value


def get_binding_value(binding, default=None):
    """Get the binding value, this function is used to deal with Undefined or
    None binding values.

    :param default: return default if value is None or Undefined
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

    if isinstance(binding, INT_TYPES):
        return _int_min_max(binding)
    elif isinstance(binding, FLOAT_TYPES):
        return _float_min_max(binding)
    # XXX: raise an exception?
    return None, None


def get_native_min_max(binding):
    """Get the native minimum and maximum of an integer or float binding

    This method does not take into account binding limits, only the native
    binding range.
    """
    if isinstance(binding, types.IntBinding):
        range_trait = binding.trait('value').handler
        value_range = range_trait._low, range_trait._high
        if range_trait._exclude_low:
            value_range = (value_range[0] + 1, value_range[1])
        if range_trait._exclude_high:
            value_range = (value_range[0], value_range[1] - 1)

        low = value_range[0]
        high = value_range[1]
        return low, high

    elif isinstance(binding, types.FloatBinding):
        attrs = binding.attributes
        value_type = attrs[const.KARABO_SCHEMA_VALUE_TYPE]
        if value_type in ('FLOAT', 'COMPLEX_FLOAT'):
            info = np.finfo(np.float32)
        else:
            info = np.finfo(np.float64)

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


# -----------------------------------------------------------------------------
# Internal functions

def _build_array_cmp(dtype):
    """Builds a comparison function for numpy arrays"""
    coerce = dtype.type
    if coerce is np.bool_:
        coerce = int

    def _array_cmp(a, b):
        try:
            return coerce(a) == coerce(b)
        except ValueError:
            return False

    return _array_cmp


def _float_min_max(binding):
    attrs = binding.attributes
    value_type = attrs.get(const.KARABO_SCHEMA_VALUE_TYPE)
    if value_type in ('FLOAT', 'COMPLEX_FLOAT'):
        info = np.finfo(np.float32)
    else:
        info = np.finfo(np.float64)

    low = attrs.get(const.KARABO_SCHEMA_MIN_EXC)
    if low is not None:
        low = low * (1 + np.sign(low) * info.eps) + info.tiny
    else:
        low = attrs.get(const.KARABO_SCHEMA_MIN_INC, info.min)

    high = attrs.get(const.KARABO_SCHEMA_MAX_EXC)
    if high is not None:
        high = high * (1 - np.sign(high) * info.eps) - info.tiny
    else:
        high = attrs.get(const.KARABO_SCHEMA_MAX_INC, info.max)

    return low, high


def _int_min_max(binding):
    range_trait = binding.trait('value').handler
    value_range = range_trait._low, range_trait._high
    if range_trait._exclude_low:
        value_range = (value_range[0] + 1, value_range[1])
    if range_trait._exclude_high:
        value_range = (value_range[0], value_range[1] - 1)

    attrs = binding.attributes
    low = attrs.get(const.KARABO_SCHEMA_MIN_EXC)
    if low is not None:
        low += 1
    else:
        low = attrs.get(const.KARABO_SCHEMA_MIN_INC, value_range[0])

    high = attrs.get(const.KARABO_SCHEMA_MAX_EXC)
    if high is not None:
        high -= 1
    else:
        high = attrs.get(const.KARABO_SCHEMA_MAX_INC, value_range[1])

    return low, high
