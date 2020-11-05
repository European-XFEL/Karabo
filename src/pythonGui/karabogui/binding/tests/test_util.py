import sys

import numpy as np

from karabo.common.api import (
    KARABO_SCHEMA_VALUE_TYPE, KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC,
    KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC, KARABO_SCHEMA_MIN_SIZE,
    KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_ABSOLUTE_ERROR,
    KARABO_SCHEMA_RELATIVE_ERROR, KARABO_SCHEMA_DISPLAYED_NAME
)
from karabo.native import (
    Configurable, Bool, Hash, HashList, Schema, String, UInt8, VectorHash)
from karabogui.binding.api import (
    BoolBinding, FloatBinding, Int8Binding, Int16Binding, Int32Binding,
    Int64Binding, Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding,
    VectorBoolBinding, VectorInt32Binding, VectorFloatBinding,
    VectorHashBinding, attr_fast_deepcopy, get_vector_hash_changes,
    get_min_max, get_min_max_size, has_changes, has_min_max_attributes,
    is_equal)
from ..util import get_vector_hash_element_changes, realign_hash


def test_simple_int_min_max():
    assert get_min_max(Int8Binding()) == (-(1 << 7), (1 << 7) - 1)
    assert get_min_max(Int16Binding()) == (-(1 << 15), (1 << 15) - 1)
    assert get_min_max(Int32Binding()) == (-(1 << 31), (1 << 31) - 1)
    assert get_min_max(Int64Binding()) == (-(1 << 63), (1 << 63) - 1)

    assert get_min_max(Uint8Binding()) == (0, (1 << 8) - 1)
    assert get_min_max(Uint16Binding()) == (0, (1 << 16) - 1)
    assert get_min_max(Uint32Binding()) == (0, (1 << 32) - 1)
    assert get_min_max(Uint64Binding()) == (0, (1 << 64) - 1)


def test_ranged_int_min_max():
    binding = Int8Binding(attributes={KARABO_SCHEMA_MIN_EXC: -7,
                                      KARABO_SCHEMA_MAX_EXC: 43})
    assert get_min_max(binding) == (-6, 42)

    binding = Int8Binding(attributes={KARABO_SCHEMA_MIN_INC: -7,
                                      KARABO_SCHEMA_MAX_INC: 43})
    assert get_min_max(binding) == (-7, 43)


def test_simple_float_min_max():
    binding = FloatBinding(attributes={KARABO_SCHEMA_VALUE_TYPE: 'DOUBLE'})
    assert get_min_max(binding) == (-sys.float_info.max, sys.float_info.max)


def test_unsupported():
    assert get_min_max(BoolBinding()) == (None, None)


def test_attr_fast_deepcopy():
    def _safe_compare(a, b):
        # Use repr() to get around the lack of Schema comparison
        return len(a) == len(b) and all(repr(a[k]) == repr(b[k]) for k in a)

    d = {
        'a': [1, 2, 3],
        'b': {'sub': 42},
        'c': 'Hi there!',
        'd': np.zeros((10,)),
        'e': (1, 2, 3),
        'f': Hash('simple', 32),
        'g': Schema()
    }
    copy = attr_fast_deepcopy(d)
    assert _safe_compare(copy, d)

    d0 = {
        KARABO_SCHEMA_DISPLAYED_NAME: 'foo',
        KARABO_SCHEMA_MIN_EXC: 0,
    }
    ref = {
        KARABO_SCHEMA_DISPLAYED_NAME: 'bar',
        KARABO_SCHEMA_MIN_EXC: 1,
    }
    # get diff between d0 and ref, KARABO_SCHEMA_DISPLAYED_NAME is not in
    # KARABO_EDITABLE_ATTRIBUTES, should not be included in the diff
    diff = attr_fast_deepcopy(d0, ref)
    assert diff == {KARABO_SCHEMA_MIN_EXC: 0}


def test_array_equal():
    assert is_equal(np.array([1, 2, 3]), np.array([1, 2, 3]))


def test_has_changes():
    binding = Int8Binding()
    assert has_changes(binding, None, 2)
    assert not has_changes(binding, 2, 2)

    binding = FloatBinding()
    assert not has_changes(binding, 1.0, 1. + 5e-8)
    assert not has_changes(binding, 2.0e10, (2 + 1.0e-7) * 1e10)
    assert has_changes(binding, 2.0, 2.0 + 3e-7)

    binding = FloatBinding(attributes={KARABO_SCHEMA_ABSOLUTE_ERROR: 0.5})
    assert not has_changes(binding, 2.0, 2.3)
    assert has_changes(binding, 2.0, 2.7)

    binding = FloatBinding(attributes={KARABO_SCHEMA_RELATIVE_ERROR: 0.25})
    assert not has_changes(binding, 2.0, 2.4)
    assert has_changes(binding, 2.0, 2.5)

    binding = VectorInt32Binding()
    assert has_changes(binding, [1, 2, 3], [1, 2])
    assert has_changes(binding, [1, 2, 3], [1, 2, 4])
    assert not has_changes(binding, [1, 2, 3], [1, 2, 3])

    assert has_changes(binding, np.array([1, 2, 3]), np.array([1, 2]))
    assert has_changes(binding, np.array([1, 2, 7]), np.array([1, 2, 3]))
    assert not has_changes(binding, np.array([1, 2, 3]), np.array([1, 2, 3]))


def test_vector_min_max():
    binding = VectorInt32Binding(attributes={KARABO_SCHEMA_MIN_SIZE: 1,
                                             KARABO_SCHEMA_MAX_SIZE: 2})
    assert get_min_max_size(binding) == (1, 2)

    binding = VectorBoolBinding(attributes={KARABO_SCHEMA_MIN_SIZE: 43})
    assert get_min_max_size(binding) == (43, None)

    binding = VectorFloatBinding(attributes={KARABO_SCHEMA_MAX_SIZE: 17})
    assert get_min_max_size(binding) == (None, 17)


def test_has_min_max():
    binding = FloatBinding(attributes={KARABO_SCHEMA_MIN_INC: 1,
                                       KARABO_SCHEMA_MAX_EXC: 3})
    assert has_min_max_attributes(binding) is True

    binding = FloatBinding(attributes={KARABO_SCHEMA_MIN_EXC: 1,
                                       KARABO_SCHEMA_MAX_EXC: 3})
    assert has_min_max_attributes(binding) is True

    binding = FloatBinding(attributes={KARABO_SCHEMA_MIN_INC: 1,
                                       KARABO_SCHEMA_MAX_INC: 3})
    assert has_min_max_attributes(binding) is True

    binding = FloatBinding(attributes={KARABO_SCHEMA_MIN_EXC: 1,
                                       KARABO_SCHEMA_MAX_INC: 3})
    assert has_min_max_attributes(binding) is True

    binding = FloatBinding(attributes={KARABO_SCHEMA_MAX_INC: 3})
    assert has_min_max_attributes(binding) is False

    binding = FloatBinding(attributes={KARABO_SCHEMA_MIN_EXC: 1})
    assert has_min_max_attributes(binding) is False

    binding = FloatBinding(attributes={KARABO_SCHEMA_MIN_EXC: 1,
                                       KARABO_SCHEMA_MIN_INC: 5})
    assert has_min_max_attributes(binding) is False

    binding = FloatBinding(attributes={KARABO_SCHEMA_MAX_EXC: 1,
                                       KARABO_SCHEMA_MAX_INC: 5})
    assert has_min_max_attributes(binding) is False


class TableRow(Configurable):
    stringProperty = String()
    uintProperty = UInt8()
    boolProperty = Bool()


def test_get_vector_hash_changes():
    binding = VectorHashBinding(row_schema=VectorHash(TableRow).rowSchema.hash)
    foo_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "boolProperty", 2)
    bar_hash = Hash("stringProperty", "bar",  # changed
                    "uintProperty", 1,
                    "boolProperty", 2)

    # Verify that there are no changes
    old_list = HashList([foo_hash])
    assert get_vector_hash_changes(binding, old_list, new=old_list) is None

    # Check changes for one hash
    new_list = HashList([bar_hash])
    changes = get_vector_hash_changes(binding, old_list, new_list)
    assert changes == new_list

    # Check changes for two hashes
    old_list = HashList([foo_hash, foo_hash])
    new_list = HashList([foo_hash, bar_hash])  # second row contains changes
    changes = get_vector_hash_changes(binding, old_list, new_list)
    # First row doesn't have changes, second row has "stringProperty" change
    assert changes == HashList([None, bar_hash])

    # Check deleted hash change on first row
    old_list = HashList([foo_hash, bar_hash])
    new_list = HashList([foo_hash])  # second hash is deleted
    changes = get_vector_hash_changes(binding, old_list, new_list)
    assert changes == new_list

    # Check deleted hash change on second row
    old_list = HashList([foo_hash, bar_hash])
    new_list = HashList([bar_hash])  # first hash is deleted
    changes = get_vector_hash_changes(binding, old_list, new_list)
    assert changes == new_list


def test_get_hash_changes():
    binding = VectorHashBinding(row_schema=VectorHash(TableRow).rowSchema.hash)
    foo_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "boolProperty", 2)
    bar_hash = Hash("stringProperty", "bar",  # changed
                    "uintProperty", 1,
                    "boolProperty", 2)

    # Check if there's no changes
    assert get_vector_hash_element_changes(binding, foo_hash, foo_hash) is None

    # Check if there's a changed property
    changes = get_vector_hash_element_changes(binding, foo_hash, bar_hash)
    assert changes == bar_hash

    # Check if there's no change for reordered hash
    expected = Hash("uintProperty", 1,
                    "stringProperty", "foo",
                    "boolProperty", 2)
    changes = get_vector_hash_element_changes(binding, foo_hash, expected)
    assert changes is None

    # Check if there's a new property added
    new_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "intProperty", 1,  # new property
                    "boolProperty", 2)
    changes = get_vector_hash_element_changes(binding, foo_hash, new_hash)
    assert changes == realign_hash(new_hash, keys=foo_hash.keys())

    # Check if there's a property removed
    new_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1)
    changes = get_vector_hash_element_changes(binding, foo_hash, new_hash)
    assert changes == Hash("stringProperty", "foo",
                           "uintProperty", 1,
                           "boolProperty", None)


def test_reorder_hash():
    # Check if ordering has not been changed
    foo_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "boolProperty", 2)
    assert realign_hash(foo_hash, keys=foo_hash.keys()) == foo_hash

    # Check if the hash has been jumbled and would still respect the keys order
    jumbled_hash = Hash("uintProperty", 1,
                        "stringProperty", "foo",
                        "boolProperty", 2)
    assert realign_hash(jumbled_hash, keys=foo_hash.keys()) == foo_hash

    # Check if the hash contains a new property than the desired keys.
    # It should appear at the end of the hash
    new_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "intProperty", 1,  # new property
                    "boolProperty", 2)
    reordered_hash = realign_hash(new_hash, keys=foo_hash.keys())
    copy_hash = Hash(foo_hash)
    copy_hash["intProperty"] = 1  # append the new property at the end
    assert reordered_hash == copy_hash

    # Check if the hash has a deleted property from the keys
    # In this canse, "uintProperty" is deleted
    new_hash = Hash("stringProperty", "foo",
                    "boolProperty", 2)
    reordered_hash = realign_hash(new_hash, keys=foo_hash.keys())
    # The deleted property contains a None to still respect the keys order
    copy_hash = Hash(foo_hash)
    copy_hash["uintProperty"] = None
    assert reordered_hash == copy_hash
