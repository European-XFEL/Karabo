import numpy as np

from karabo.common.api import (
    KARABO_SCHEMA_ABSOLUTE_ERROR, KARABO_SCHEMA_DISPLAYED_NAME,
    KARABO_SCHEMA_RELATIVE_ERROR, KARABO_WARN_LOW)
from karabo.native import (
    Bool, Configurable, Hash, HashList, Schema, String, UInt8, VectorHash)
from karabogui.binding.api import (
    FloatBinding, Int8Binding, VectorHashBinding, VectorInt32Binding,
    attr_fast_deepcopy, get_table_changes, has_array_changes, has_changes,
    realign_hash, table_row_changes)


def test_attr_fast_deepcopy():
    # Note: This is as well in MDL conf. Find proper place!
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
        KARABO_WARN_LOW: 1.0,
    }
    ref = {
        KARABO_SCHEMA_DISPLAYED_NAME: 'bar',
        KARABO_WARN_LOW: 1.2,
    }
    # get diff between d0 and ref, KARABO_SCHEMA_DISPLAYED_NAME is not in
    # KARABO_EDITABLE_ATTRIBUTES, should not be included in the diff
    diff = attr_fast_deepcopy(d0, ref)
    assert diff == {KARABO_WARN_LOW: 1.0}


def test_array_equal():
    assert not has_array_changes(np.array([1, 2, 3]), np.array([1, 2, 3]))


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

    binding = FloatBinding()
    assert not has_changes(binding, 0, 0)
    assert has_changes(binding, 0, 1e-9)

    binding = VectorInt32Binding()
    assert has_changes(binding, [1, 2, 3], [1, 2])
    assert has_changes(binding, [1, 2, 3], [1, 2, 4])
    assert not has_changes(binding, [1, 2, 3], [1, 2, 3])

    assert has_changes(binding, np.array([1, 2, 3]), np.array([1, 2]))
    assert has_changes(binding, np.array([1, 2, 7]), np.array([1, 2, 3]))
    assert not has_changes(binding, np.array([1, 2, 3]), np.array([1, 2, 3]))


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
    assert get_table_changes(binding, old_list, new=old_list) is None

    # Check changes for one hash
    new_list = HashList([bar_hash])
    changes = get_table_changes(binding, old_list, new_list)
    assert changes == new_list

    # Check changes for two hashes
    old_list = HashList([foo_hash, foo_hash])
    new_list = HashList([foo_hash, bar_hash])  # second row contains changes
    changes = get_table_changes(binding, old_list, new_list)
    # First row doesn't have changes, second row has "stringProperty" change
    assert changes == HashList([None, bar_hash])

    # Check deleted hash change on first row
    old_list = HashList([foo_hash, bar_hash])
    new_list = HashList([foo_hash])  # second hash is deleted
    changes = get_table_changes(binding, old_list, new_list)
    assert changes == new_list

    # Check deleted hash change on second row
    old_list = HashList([foo_hash, bar_hash])
    new_list = HashList([bar_hash])  # first hash is deleted
    changes = get_table_changes(binding, old_list, new_list)
    assert changes == new_list


def test_table_row_changes():
    binding = VectorHashBinding(row_schema=VectorHash(TableRow).rowSchema.hash)
    foo_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "boolProperty", 2)
    bar_hash = Hash("stringProperty", "bar",  # changed
                    "uintProperty", 1,
                    "boolProperty", 2)

    # Check if there's no changes
    assert table_row_changes(binding, foo_hash, foo_hash) is None

    # Check if there's a changed property
    changes = table_row_changes(binding, foo_hash, bar_hash)
    assert changes == bar_hash

    # Check if there's no change for reordered hash
    expected = Hash("uintProperty", 1,
                    "stringProperty", "foo",
                    "boolProperty", 2)
    changes = table_row_changes(binding, foo_hash, expected)
    assert changes is None

    # Check if there's a new property added
    new_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "intProperty", 1,  # new property
                    "boolProperty", 2)
    changes = table_row_changes(binding, foo_hash, new_hash)
    assert changes is None  # `intProperty` is not applied

    # Check if there's a property removed
    new_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1)
    changes = table_row_changes(binding, foo_hash, new_hash)
    assert changes == Hash("stringProperty", "foo",
                           "uintProperty", 1,
                           "boolProperty", None)


def test_realign_hash():
    """Test the realign hash function"""

    # 1. Check when ordering was not been changed
    foo_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "boolProperty", 2)
    reference = foo_hash.keys()

    assert realign_hash(foo_hash, reference=reference) == foo_hash

    # 2. Check when the hash was jumbled and would still respect the keys order
    jumbled_hash = Hash("uintProperty", 1,
                        "stringProperty", "foo",
                        "boolProperty", 2)
    assert realign_hash(jumbled_hash, reference=reference) == foo_hash

    # 3. Check if the hash contains a new property with respect to reference
    # It must be appended appear at the end of the hash
    new_hash = Hash("stringProperty", "foo",
                    "uintProperty", 1,
                    "intProperty", 1,  # new property
                    "boolProperty", 2)

    reordered_hash = realign_hash(new_hash, reference=reference)
    copy_hash = Hash(foo_hash)
    copy_hash["intProperty"] = 1  # append the new property at the end
    assert reordered_hash == copy_hash

    # 4. Check if the hash has a deleted property from the keys
    # In the current case, `uintProperty` is removed
    new_hash = Hash("stringProperty", "foo",
                    "boolProperty", 2)
    reordered_hash = realign_hash(new_hash, reference=reference)
    # The removed property gets value of `None` but the Hash still respects
    # the reference key order
    copy_hash = Hash(foo_hash)
    copy_hash["uintProperty"] = None
    assert reordered_hash == copy_hash
