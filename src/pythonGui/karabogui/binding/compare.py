import numbers
from itertools import zip_longest

import numpy as np

from karabo.common import const
from karabo.native import AccessMode, Hash, HashList, is_equal, simple_deepcopy

FLOAT_TOLERANCE = 1e-7
ZERO_FLOAT_TOLERANCE = 1e-9


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
        out[k] = simple_deepcopy(v)

    return out


def is_nonintegral_number(value):
    """Check if `value` is not an integral number, common cases are floats.
    """
    is_floating = isinstance(value, (numbers.Complex, np.inexact))
    is_integer = isinstance(value, numbers.Integral)
    return is_floating and not is_integer


def has_changes(binding, old_value, new_value):
    """Compare old/new values assigned to a binding to determine if there is
    a real difference.
    """
    try:
        if old_value is None:
            changes = True
        elif is_nonintegral_number(old_value):
            changes = has_floating_changes(binding, old_value, new_value)
        elif isinstance(old_value, np.ndarray):
            changes = has_array_changes(old_value, new_value)
        elif isinstance(old_value, HashList):
            changes = has_table_changes(binding, old_value, new_value)
        elif isinstance(old_value, list):
            changes = has_list_changes(old_value, new_value)
        else:
            changes = (str(old_value) != str(new_value))
    except TypeError:
        # When old_value and new_value have different types and cannot be
        # operated/compared together, it signifies that there are changes.
        changes = True

    return changes


def has_floating_changes(binding, old, new):
    """Check if there are floating point differences between `old` and `new`

    This function complies to the `IEEE 754` specification.
    """
    abs_err, rel_err = None, None
    if binding is not None:
        attrs = binding.attributes
        abs_err = attrs.get(const.KARABO_SCHEMA_ABSOLUTE_ERROR)
        rel_err = attrs.get(const.KARABO_SCHEMA_RELATIVE_ERROR)

    difference = abs(old - new)
    if abs_err is not None:
        changes = (difference >= abs_err)
    elif rel_err is not None:
        changes = (difference >= abs(old * rel_err))
    else:
        if not old:
            # Note: In case of a previous 0 value we must ask differently
            changes = (difference >= ZERO_FLOAT_TOLERANCE)
        else:
            changes = (difference >= abs(old * FLOAT_TOLERANCE))

    return changes


def has_array_changes(old, new, dtype=False):
    """Compare if arrays are equal with a floating point tolerance (IEEE 754)
    """
    if len(old) != len(new):
        return True

    if dtype and old.dtype != new.dtype:
        return True

    changes = not np.allclose(old, new, atol=FLOAT_TOLERANCE)

    return changes


def has_list_changes(old, new):
    """Compare if lists have changes with `is_equal`"""
    if len(old) != len(new):
        return True
    changes = False
    for i in range(len(old)):
        if not is_equal(old[i], new[i]):
            changes = True
            break
    return changes


def has_table_changes(binding, old, new):
    """Check if a table element has changes

    :param binding: The `VectorHash` binding
    :param old, new: The `HashLists` to be compared
    """
    changes = get_table_changes(binding, old, new)
    return changes is not None


def get_table_changes(binding, old, new):
    """Returns the diff of the old hash and the new hash.

    :param binding: The `VectorHash` binding
    :param old, new: (HashList or None) to be compared

    :return changes: (HashList or None)

            Returns `HashList` if there are changes, `None` otherwise.
            The values could contain [None, Hash()]
    """

    changes = HashList()
    if old is None:
        return new

    iter_hashes = zip_longest(old, new)
    for old_row, new_row in iter_hashes:
        # The hash is deleted, we don't record
        if new_row is None:
            continue
        changes.append(table_row_changes(binding, old_row, new_row))

    if changes.count(None) == len(changes):
        # No changes if all of the values are `None` and the number of rows is
        # the same. Else, rows are deleted and we use the new value instead.
        changes = None if len(old) == len(changes) else new

    return changes


def table_row_changes(binding, old_row, new_row):
    """Returns the changes of old row and new row hash

    :param binding: The `VectorHash` binding
    :param old_row, new_row: (HashList or None) to be compared

    :return `Hash` or `None`: Hash if there are changes, None otherwise
    """
    if old_row is None:
        # There's no prior value, we return the new row hash immediately
        return new_row
    elif new_row is None:
        # The hash is deleted, we return an empty hash
        return Hash()

    # Check if order of the keys are respected
    if list(old_row.keys()) != list(new_row.keys()):
        new_row = realign_hash(new_row, reference=old_row.keys())

    # Check over if there are changes by iterating and comparing the hashes
    row_schema = binding.bindings
    iter_column = zip_longest(old_row.items(), new_row.items(),
                              fillvalue=(None, None))
    for (old_key, old_value), (new_key, new_value) in iter_column:
        prop_binding = row_schema.get(new_key)
        # XXX: A binding that is not available is neglected ...
        if (new_key is None or prop_binding is None
                or prop_binding.access_mode is AccessMode.READONLY):
            continue
        if (old_key is None or has_changes(
                prop_binding, old_value, new_value)):
            # There are changes and it can be written! Hence, return the
            # whole row hash.
            return new_row

    # No changes detected, we return None!
    return None


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
