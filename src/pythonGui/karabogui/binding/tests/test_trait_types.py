import numpy as np
import pytest
from traits.api import TraitError

from ..api import (
    Int8Binding, Int16Binding, Int32Binding, Int64Binding, Uint8Binding,
    Uint16Binding, Uint32Binding, Uint64Binding, FloatBinding)


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
        assert type(value) == dtype
        value = binding.validate_trait("value", "23")
        assert value == 23
        assert type(value) == dtype
        value = binding.validate_trait("value", 2)
        assert value == 2
        assert type(value) == dtype
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
        with pytest.raises(TraitError):
            binding.validate_trait("value", -1)


def test_validate_float():
    """Test if the coerce of the float binding"""

    binding = FloatBinding()
    value = binding.validate_trait("value", 23)
    assert value == 23.0
    assert type(value) == float
    value = binding.validate_trait("value", "23")
    assert value == 23
    assert type(value) == float
    value = binding.validate_trait("value", np.float32(4))
    assert value == 4.0
    assert type(value) == np.float32
    value = binding.validate_trait("value", np.float64(4))
    assert value == 4.0
    assert type(value) == np.float64
