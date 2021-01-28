from collections.abc import Iterable
from copy import deepcopy
from itertools import zip_longest
import numbers

import numpy as np
from traits.api import Undefined

from karabo.common import const
from karabo.common.api import (
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC)
from karabo.native import AccessMode, Hash, HashList, Schema
from . import types

FLOAT_TOLERANCE = 1e-7


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


def has_changes(binding, old_value, new_value, init=False):
    """Compare old/new values assigned to a binding to determine if there is
    a real difference.

    :param init: Check if init only parameter are considered. This is used
                 only for binding of a table element
    """
    # Check if changes were made
    try:
        if old_value is None:
            changes = True
        elif is_nonintegral_number(old_value):
            abs_err, rel_err = None, None
            if binding is not None:
                attrs = binding.attributes
                abs_err = attrs.get(const.KARABO_SCHEMA_ABSOLUTE_ERROR)
                rel_err = attrs.get(const.KARABO_SCHEMA_RELATIVE_ERROR)
            diff = abs(old_value - new_value)
            if abs_err is not None:
                changes = (diff >= abs_err)
            elif rel_err is not None:
                changes = (diff >= abs(old_value * rel_err))
            else:
                # IEEE 754 compliance
                changes = (diff >= abs(old_value * FLOAT_TOLERANCE))
        elif isinstance(old_value, np.ndarray):
            changes = not array_equal(new_value, old_value)
        elif isinstance(old_value, HashList):
            changes = has_vector_hash_changes(binding, old_value, new_value,
                                              init=init)
        elif isinstance(old_value, list):
            if len(old_value) != len(new_value):
                changes = True
            else:
                changes = False
                for i in range(len(old_value)):
                    if not is_equal(old_value[i], new_value[i]):
                        changes = True
                        break
        else:
            changes = (str(old_value) != str(new_value))
    except TypeError:
        # When old_value and new_value have different types and cannot be
        # operated/compared together, it signifies that there are changes.
        changes = True

    return changes


def is_nonintegral_number(number):
    # Check a value if it is not an integral number. Common cases are floats.
    return (isinstance(number, (numbers.Complex, np.inexact))
            and not isinstance(number, numbers.Integral))


def has_diff(old_value, new_value, abs_err=None, rel_err=None):
    diff = abs(old_value - new_value)
    if abs_err is not None:
        return diff >= abs_err
    elif rel_err is not None:
        return diff >= abs(old_value * rel_err)
    else:
        return diff >= abs(old_value * 1e-7)  # IEEE 754 compliance


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


def array_equal(actual, expected):
    """Compare if arrays are equal with a floating point tolerance (IEEE 754)
    """
    if len(actual) != len(expected):
        return False

    return np.allclose(actual, expected, atol=FLOAT_TOLERANCE)


def has_vector_hash_changes(binding, old, new, init=False):
    """Check if a table element has changes

    :param binding: The VectorHash binding
    :param old, new: The vector hashes to be compared
    :param init: (bool) Denote if `INITONLY` properties are also valid
    """
    hash_list = get_vector_hash_changes(binding, old, new, init)
    return hash_list is not None


def get_vector_hash_changes(binding, old, new, init=False):
    """Returns the diff of the old hash and the new hash.

    :param binding: (VectorHashBinding) the binding which contains
        the current value and the row schema
    :param old, new: (Hash or None) the hashes to be compared
    :param init: (bool) Denote if `INITONLY` properties are also valid

    :return changes: (HashList or None)

            Returns HashList if there are changes, None otherwise.
            The values could contain [None, Hash()]
    """
    changes = HashList()
    if old is None:
        return new

    iter_hashes = zip_longest(old, new)
    for old_hash, new_hash in iter_hashes:
        # The hash is deleted, we don't record
        if new_hash is None:
            continue
        change = get_vector_hash_element_changes(binding, old_hash, new_hash,
                                                 init)
        changes.append(change)

    if changes.count(None) == len(changes):
        # No changes if all of the values are Nones and the number of rows is
        # the same. Else, rows are deleted and we use the new value instead.
        changes = None if len(old) == len(changes) else new

    return changes


def get_vector_hash_element_changes(binding, old, new, init=False):
    """Returns the diff of the old hash and the new hash.

    :param binding: (VectorHashBinding) the binding which contains
                     the current value and the row schema
    :param old, new: (Hash or None) the hashes to be compared

    :return (Hash or None): returns hash if there's changes, or None otherwise
    """
    if old is None:
        # There's no prior value, we return the new hash immediately
        return new

    if new is None:
        # The hash is deleted, we return an empty hash
        return Hash()

    # Check if order of the keys are respected
    if list(old.keys()) != list(new.keys()):
        new = realign_hash(new, keys=old.keys())

    # Check over if there are changes by iterating and comparing the hashes
    column_bindings = binding.bindings
    iter_prop = zip_longest(old.items(), new.items(),
                            fillvalue=(None, None))
    for (old_name, old_value), (new_name, new_value) in iter_prop:
        prop_binding = column_bindings.get(new_name)
        if new_name is None or not is_writable(prop_binding, init):
            continue
        if (old_name is None or has_changes(
                prop_binding, old_value, new_value)):
            # There are changes and it can be written!
            # Hence, we now return the whole hash.
            return new

    # No changes detected, we return None
    return None


def is_writable(binding, init=False):
    """Check if a binding is reconfigurable with the attributes

    :param init: boolean to consider `INITONLY` properties. Default is `False`.
    """
    if binding is None:
        return False

    # Allow init only values when merging
    access_modes = (AccessMode.RECONFIGURABLE,)
    if init:
        access_modes += (AccessMode.INITONLY,)
    return binding.access_mode in access_modes


def realign_hash(hsh, keys):
    """Realigns the hash `hsh` according to the key order `keys`.

    Fills properties with `None` if key is not present in `keys`. The
    properties not in the `keys` are appended at the end.

    :param hsh: (Hash) dictionary of bindings {path: binding}
    :param keys: (sequence) a list of `keys` as reference

    :return (Hash): returns a new realigned `Hash` (copy)
    """

    ret = Hash()
    for old_key in keys:
        if old_key in hsh:
            ret[old_key] = deepcopy(hsh[old_key])
            ret[old_key, ...] = deepcopy(hsh[old_key, ...])
        else:
            ret[old_key] = None
            ret[old_key, ...] = {}

    # Now, we include the hsh keys (not existing on the old hash)
    # We expect a purely flat Hash here!
    for key, value, attrs in Hash.flat_iterall(hsh):
        if key in keys:
            # Already considered
            continue
        ret[key] = deepcopy(value)
        ret[key, ...] = deepcopy(attrs)

    return ret
