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
import sys

import numpy as np

from karabo.common.api import (
    KARABO_SCHEMA_DISPLAY_TYPE, KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC,
    KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC,
    KARABO_SCHEMA_MIN_SIZE, KARABO_SCHEMA_VALUE_TYPE)
from karabo.native import Hash, Schema
from karabogui.binding.api import (
    BoolBinding, FloatBinding, Int8Binding, Int16Binding, Int32Binding,
    Int64Binding, StringBinding, Uint8Binding, Uint16Binding, Uint32Binding,
    Uint64Binding, VectorBoolBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorHashBinding, VectorInt8Binding, VectorInt16Binding,
    VectorInt32Binding, VectorInt64Binding, VectorStringBinding,
    VectorUint8Binding, VectorUint16Binding, VectorUint32Binding,
    VectorUint64Binding, attr_fast_deepcopy, get_dtype_format, get_min_max,
    get_min_max_size, get_native_min_max, has_min_max_attributes,
    is_signed_vector_integer, is_unsigned_vector_integer, is_vector_integer,
    realign_hash)


def test_simple_int_min_max():
    assert get_min_max(Int8Binding()) == (-(1 << 7), (1 << 7) - 1)
    assert get_min_max(Int16Binding()) == (-(1 << 15), (1 << 15) - 1)
    assert get_min_max(Int32Binding()) == (-(1 << 31), (1 << 31) - 1)
    assert get_min_max(Int64Binding()) == (-(1 << 63), (1 << 63) - 1)

    assert get_min_max(Uint8Binding()) == (0, (1 << 8) - 1)
    assert get_min_max(Uint16Binding()) == (0, (1 << 16) - 1)
    assert get_min_max(Uint32Binding()) == (0, (1 << 32) - 1)
    assert get_min_max(Uint64Binding()) == (0, (1 << 64) - 1)


def test_native_int_min_max():
    assert get_native_min_max(Int8Binding()) == (-(1 << 7), (1 << 7) - 1)
    assert get_native_min_max(Int16Binding()) == (-(1 << 15), (1 << 15) - 1)
    assert get_native_min_max(Int32Binding()) == (-(1 << 31), (1 << 31) - 1)
    assert get_native_min_max(Int64Binding()) == (-(1 << 63), (1 << 63) - 1)

    assert get_native_min_max(Uint8Binding()) == (0, (1 << 8) - 1)
    assert get_native_min_max(Uint16Binding()) == (0, (1 << 16) - 1)
    assert get_native_min_max(Uint32Binding()) == (0, (1 << 32) - 1)
    assert get_native_min_max(Uint64Binding()) == (0, (1 << 64) - 1)

    # Binding specifications are neglected
    binding = Int8Binding(attributes={
        KARABO_SCHEMA_VALUE_TYPE: 'INT8',
        KARABO_SCHEMA_MIN_EXC: -7,
        KARABO_SCHEMA_MAX_EXC: 43})
    assert get_native_min_max(binding) == (-(1 << 7), (1 << 7) - 1)


def test_native_vector_min_max():
    test_map = [
        (VectorInt8Binding, (-(1 << 7), (1 << 7) - 1)),
        (VectorInt16Binding, (-(1 << 15), (1 << 15) - 1)),
        (VectorInt32Binding, (-(1 << 31), (1 << 31) - 1)),
        (VectorInt64Binding, (-(1 << 63), (1 << 63) - 1)),

        (VectorUint8Binding, (0, (1 << 8) - 1)),
        (VectorUint16Binding, (0, (1 << 16) - 1)),
        (VectorUint32Binding, (0, (1 << 32) - 1)),
        (VectorUint64Binding, (0, (1 << 64) - 1))]
    for binding, expected in test_map:
        assert get_native_min_max(binding()) == expected


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
    assert get_native_min_max(binding) == (-sys.float_info.max,
                                           sys.float_info.max)


def test_unsupported():
    assert get_min_max(BoolBinding()) == (None, None)
    assert get_native_min_max(BoolBinding()) == (None, None)


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


def test_display_format():
    assert get_dtype_format(None) == "{}"

    binding = FloatBinding()
    assert get_dtype_format(binding) == "{:.8g}"

    binding = Int8Binding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "bin"})
    assert get_dtype_format(binding) == "0b{:b}"

    binding = Int8Binding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "hex"})
    assert get_dtype_format(binding) == "0x{:X}"

    binding = Int8Binding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "oct"})
    assert get_dtype_format(binding) == "o{:o}"

    binding = Int8Binding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "oct|||"})
    assert get_dtype_format(binding) == "o{:o}"

    binding = Int8Binding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "moct"})
    assert get_dtype_format(binding) == "{}"

    # Only accounted for IntegerBindings
    binding = StringBinding(attributes={KARABO_SCHEMA_DISPLAY_TYPE: "oct"})
    assert get_dtype_format(binding) == "{}"


def test_integer_binding():
    # Signed bindings
    assert is_signed_vector_integer(VectorInt8Binding())
    assert is_signed_vector_integer(VectorInt16Binding())
    assert is_signed_vector_integer(VectorInt32Binding())
    assert is_signed_vector_integer(VectorInt64Binding())

    # Unsigned bindings
    assert is_unsigned_vector_integer(VectorUint8Binding())
    assert is_unsigned_vector_integer(VectorUint16Binding())
    assert is_unsigned_vector_integer(VectorUint32Binding())
    assert is_unsigned_vector_integer(VectorUint64Binding())

    # All int bindings
    assert is_vector_integer(VectorInt8Binding())
    assert is_vector_integer(VectorInt16Binding())
    assert is_vector_integer(VectorInt32Binding())
    assert is_vector_integer(VectorInt64Binding())
    assert is_vector_integer(VectorUint8Binding())
    assert is_vector_integer(VectorUint16Binding())
    assert is_vector_integer(VectorUint32Binding())
    assert is_vector_integer(VectorUint64Binding())

    assert not is_vector_integer(VectorBoolBinding())
    assert not is_vector_integer(VectorStringBinding())
    assert not is_vector_integer(VectorHashBinding())
    assert not is_vector_integer(VectorDoubleBinding())
