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
import numbers

import numpy as np

from .hash import Hash, is_equal

FLOAT_TOLERANCE = 1e-7
ZERO_FLOAT_TOLERANCE = 1e-9


def has_changes(old_value, new_value):
    """Compare old/new values to determine if there is a real difference

    This function has been added in Karabo >= 2.14.
    """
    try:
        if old_value is None:
            changes = new_value is not None
        elif _is_nonintegral_number(old_value):
            changes = _has_floating_changes(old_value, new_value)
        elif isinstance(old_value, np.ndarray):
            changes = _has_array_changes(old_value, new_value)
        elif isinstance(old_value, list):
            changes = _has_list_changes(old_value, new_value)
        else:
            changes = str(old_value) != str(new_value)
    except TypeError:
        # When old_value and new_value have different types and cannot be
        # operated/compared together, it signifies that there are changes.
        changes = True

    return changes


def _is_nonintegral_number(value):
    """Check if `value` is not an integral number, common cases are floats"""
    is_floating = isinstance(value, (numbers.Complex, np.inexact))
    is_integer = isinstance(value, numbers.Integral)
    return is_floating and not is_integer


def _has_floating_changes(old, new):
    """Check if there are floating point differences between `old` and `new`

    This function complies to the `IEEE 754` specification.
    """
    difference = abs(old - new)
    if not old:
        # Note: In case of a previous 0 value we must ask differently
        changes = difference >= ZERO_FLOAT_TOLERANCE
    else:
        changes = difference >= abs(old * FLOAT_TOLERANCE)

    return changes


def _has_array_changes(old, new):
    """Compare if arrays are equal with a floating point tolerance (IEEE 754)
    """
    if len(old) != len(new):
        return True
    if not isinstance(new, np.ndarray):
        return True
    if (np.issubdtype(old.dtype, np.floating)
            or np.issubdtype(new.dtype, np.floating)):
        return not np.allclose(old, new, atol=FLOAT_TOLERANCE)

    # In case of non-floating arrays, we use `array_equal`
    return not np.array_equal(old, new)


def _has_hash_changes(old, new):
    """Compare if two Hashes have changes in values with `has_changes`"""
    old_paths = sorted(old.paths(intermediate=True))
    new_paths = sorted(new.paths(intermediate=True))
    if old_paths != new_paths:
        return True

    iterable = Hash.flat_iterall(new, empty=True)
    for key, value, _ in iterable:
        old_value = old[key]
        if has_changes(old_value, value):
            return True

    return False


def _has_list_changes(old, new):
    """Compare if lists have changes with `is_equal` or `_has_hash_changes` for
    vector hashes"""
    if len(old) != len(new):
        return True

    for i in range(len(old)):
        new_value = new[i]
        old_value = old[i]
        if isinstance(old_value, Hash) and isinstance(new_value, Hash):
            if _has_hash_changes(old_value, new_value):
                return True
        elif not is_equal(old_value, new_value):
            return True

    return False
