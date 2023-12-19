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
import numpy as np
import pytest
from traits.api import TraitError

from karabogui.binding.api import (
    BoolBinding, FloatBinding, Int8Binding, Int16Binding, Int32Binding,
    Int64Binding, Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding)


def test_validate_integers():
    """Test the coerce of our numpy integer range"""

    bindings = {
        Int64Binding: np.int64,
        Int32Binding: np.int32,
        Int16Binding: np.int16,
        Int8Binding: np.int8,
        Uint64Binding: np.uint64,
        Uint32Binding: np.uint32,
        Uint16Binding: np.uint16,
        Uint8Binding: np.uint8,
    }

    for property_binding, dtype in bindings.items():
        binding = property_binding()
        value = binding.validate_trait("value", dtype(23))
        assert value == 23
        assert isinstance(value, dtype)
        value = binding.validate_trait("value", "23")
        assert value == 23
        assert isinstance(value, dtype)
        value = binding.validate_trait("value", 2)
        assert value == 2
        assert isinstance(value, dtype)
        # Put a goofy value and check raise
        with pytest.raises(TraitError):
            binding.validate_trait("value", [])

    # Check basic limits for unsigned ints again ...
    bindings = {
        Uint64Binding: np.uint64,
        Uint32Binding: np.uint32,
        Uint16Binding: np.uint16,
        Uint8Binding: np.uint8,
    }
    for property_binding in bindings.keys():
        binding = property_binding()
        negative_value = -1
        with pytest.raises(TraitError):
            binding.validate_trait("value", negative_value)

        negative_overflow = -2e100
        with pytest.raises(TraitError):
            binding.validate_trait("value", str(negative_overflow))
        with pytest.raises(TraitError):
            binding.validate_trait("value", negative_overflow)

        positive_overflow = 2e100
        with pytest.raises(TraitError):
            binding.validate_trait("value", str(positive_overflow))
        with pytest.raises(TraitError):
            binding.validate_trait("value", positive_overflow)


def test_validate_boolean():
    """Test if the coerce of the bool binding"""

    binding = BoolBinding()
    value = binding.validate_trait("value", 23)
    assert value is True
    assert isinstance(value, bool)
    value = binding.validate_trait("value", "23")
    assert value is True
    assert isinstance(value, bool)
    value = binding.validate_trait("value", False)
    assert value is False
    assert isinstance(value, bool)
    value = binding.validate_trait("value", True)
    assert value is True
    assert isinstance(value, bool)
    value = binding.validate_trait("value", 0)
    assert value is False
    assert isinstance(value, bool)
    value = binding.validate_trait("value", 1)
    assert value is True
    assert isinstance(value, bool)
    with pytest.raises(Exception):
        binding.validate_trait("value", "None")
    with pytest.raises(Exception):
        binding.validate_trait("value", None)


def test_validate_float():
    """Test if the coerce of the float binding"""

    binding = FloatBinding()
    value = binding.validate_trait("value", 23)
    assert value == 23.0
    assert isinstance(value, float)
    value = binding.validate_trait("value", "23")
    assert value == 23
    assert isinstance(value, float)
    value = binding.validate_trait("value", np.float32(4))
    assert value == 4.0
    assert isinstance(value, np.float32)
    value = binding.validate_trait("value", np.float64(4))
    assert value == 4.0
    assert isinstance(value, np.float64)


def test_numpy_binding_types():
    """Test if our numpy value type is preserved"""
    int64Property = Int64Binding()
    int64Property.value = np.int64(23)
    assert int64Property.value == 23
    assert isinstance(int64Property.value, np.int64)
    int64Property.value = 64
    assert int64Property.value == 64
    assert isinstance(int64Property.value, np.int64)
    int64Property.value = 23
    assert int64Property.value == 23
    assert isinstance(int64Property.value, np.int64)

    int32Property = Int32Binding()
    int32Property.value = np.int32(23)
    assert int32Property.value == 23
    assert isinstance(int32Property.value, np.int32)
    int32Property.value = 64
    assert int32Property.value == 64
    assert isinstance(int32Property.value, np.int32)
    int32Property.value = np.int16(16)
    assert int32Property.value == 16
    assert isinstance(int32Property.value, np.int32)

    int16Property = Int16Binding()
    int16Property.value = np.int16(23)
    assert int16Property.value == 23
    assert isinstance(int16Property.value, np.int16)
    int16Property.value = 64
    assert int16Property.value == 64
    assert isinstance(int16Property.value, np.int16)
    int16Property.value = np.int8(16)
    assert int16Property.value == 16
    assert isinstance(int16Property.value, np.int16)

    int8Property = Int8Binding()
    int8Property.value = np.int8(23)
    assert int8Property.value == 23
    assert isinstance(int8Property.value, np.int8)
    int8Property.value = 64
    assert int8Property.value == 64
    assert isinstance(int8Property.value, np.int8)
    int8Property.value = np.int16(16)
    assert int8Property.value == 16
    assert isinstance(int8Property.value, np.int8)

    uint64Property = Uint64Binding()
    uint64Property.value = np.uint64(23)
    assert uint64Property.value == 23
    assert isinstance(uint64Property.value, np.uint64)
    uint64Property.value = 64
    assert uint64Property.value == 64
    assert isinstance(uint64Property.value, np.uint64)
    uint64Property.value = np.int16(16)
    assert uint64Property.value == 16
    assert isinstance(uint64Property.value, np.uint64)

    uint32Property = Uint32Binding()
    uint32Property.value = np.uint32(23)
    assert uint32Property.value == 23
    assert isinstance(uint32Property.value, np.uint32)
    uint32Property.value = 64
    assert uint32Property.value == 64
    assert isinstance(uint32Property.value, np.uint32)
    uint32Property.value = np.int16(16)
    assert uint32Property.value == 16
    assert isinstance(uint32Property.value, np.uint32)

    uint16Property = Uint16Binding()
    uint16Property.value = np.uint16(23)
    assert uint16Property.value == 23
    assert isinstance(uint16Property.value, np.uint16)
    uint16Property.value = 64
    assert uint16Property.value == 64
    assert isinstance(uint16Property.value, np.uint16)

    uint8Property = Uint8Binding()
    uint8Property.value = np.uint8(23)
    assert uint8Property.value == 23
    assert isinstance(uint8Property.value, np.uint8)
    uint8Property.value = 64
    assert uint8Property.value == 64
    assert isinstance(uint8Property.value, np.uint8)
