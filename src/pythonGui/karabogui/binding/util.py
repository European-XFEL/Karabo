from collections.abc import Iterable
import numbers

import numpy as np
from traits.api import Undefined

from karabo.common import const
from karabo.middlelayer import Hash, Schema
from . import types


def attr_fast_deepcopy(d, ref=None):
    """copy.deepcopy is criminally slow. We can bypass its fanciness as long
    as we only copy 'simple' datastructures.

    Pass a not None attributes dict to `ref` to get only changed attributes
    """
    out = {}

    for k, v in d.items():
        if ref is not None:
            if (k not in const.KARABO_EDITABLE_ATTRIBUTES or
                    is_equal(v, ref.get(k))):
                continue
        try:
            out[k] = v.copy()  # dicts, sets, ndarrays
        except TypeError:
            # This is a Schema, which has a terrible API
            assert isinstance(v, Schema)
            cpy = Schema()
            cpy.copy(v)
            out[k] = cpy
        except AttributeError:
            try:
                out[k] = v[:]  # lists, tuples, strings, unicode
            except TypeError:
                out[k] = v  # simple values

    return out


def flat_iter_hash(h, base=''):
    """Recursively iterate over all the values in a Hash object such that
    a simple iterator interface is exposed. In this way, a single for-loop
    is sufficient to see an entire Hash.
    """
    base = base + '.' if base else ''
    for key, value, attrs in h.iterall():
        here = base + key
        if isinstance(value, Hash):
            yield from flat_iter_hash(value, base=here)
        else:
            yield here, value, attrs


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


def get_min_max_size(binding):
    """Given a BaseBinding instance, return the minimum and maximum size
    which can be assigned to the vector trait
    """
    if isinstance(binding, types.VectorBinding):
        min_size = binding.attributes.get(const.KARABO_SCHEMA_MIN_SIZE)
        max_size = binding.attributes.get(const.KARABO_SCHEMA_MAX_SIZE)

        return min_size, max_size

    return None, None


def has_changes(binding, old_value, new_value):
    """Compare old/new values assigned to a binding to determine if there is
    a real difference.
    """
    # Check if changes were made
    if old_value is None:
        changes = True
    elif (isinstance(old_value, (numbers.Complex, np.inexact))
            and not isinstance(old_value, numbers.Integral)):
        diff = abs(old_value - new_value)
        abs_err = binding.attributes.get(const.KARABO_SCHEMA_ABSOLUTE_ERROR)
        rel_err = binding.attributes.get(const.KARABO_SCHEMA_RELATIVE_ERROR)
        if abs_err is not None:
            changes = (diff >= abs_err)
        elif rel_err is not None:
            changes = (diff >= abs(old_value * rel_err))
        else:
            changes = (diff >= abs(old_value * 1e-7))  # IEEE 754 compliance
    elif isinstance(old_value, (list, np.ndarray)):
        if len(old_value) != len(new_value):
            changes = True
        else:
            changes = False
            cmp = is_equal
            if isinstance(old_value, np.ndarray):
                cmp = _build_array_cmp(old_value.dtype)
            for i in range(len(old_value)):
                if not cmp(old_value[i], new_value[i]):
                    changes = True
                    break
    else:
        changes = (str(old_value) != str(new_value))
    return changes


def is_equal(a, b):
    """A compare function deals with element-wise comparison result and
    Schema object comparison
    """
    type_check = map(lambda x: isinstance(x, Schema), (a, b))
    if any(type_check):
        if all(type_check):
            # Compare Schema objects' names and hashes
            return a.name == b.name and a.hash == b.hash
        else:
            # one of a, b is not Schema, simply return False
            return False
    res = (a == b)
    # comparison of numpy arrays result in an array
    return all(res) if isinstance(res, Iterable) else res


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
