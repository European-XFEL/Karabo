import numpy as np

from karabo.common.api import KARABO_SCHEMA_DISPLAYED_NAME, KARABO_WARN_LOW
from karabo.native import Hash, Schema
from karabogui.binding.api import attr_fast_deepcopy, realign_hash


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
