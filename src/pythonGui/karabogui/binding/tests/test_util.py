import sys

from karabo.common.api import (
    KARABO_SCHEMA_VALUE_TYPE, KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC,
    KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC, KARABO_SCHEMA_MIN_SIZE,
    KARABO_SCHEMA_MAX_SIZE)
from karabogui.binding.api import (
    BoolBinding, FloatBinding, Int8Binding, Int16Binding, Int32Binding,
    Int64Binding, Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding,
    VectorBoolBinding, VectorInt32Binding, VectorFloatBinding,
    get_native_min_max, get_min_max, get_min_max_size, has_min_max_attributes)


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
    binding = Int8Binding(attributes={KARABO_SCHEMA_MIN_EXC: -7,
                                      KARABO_SCHEMA_MAX_EXC: 43})
    assert get_native_min_max(binding) == (-(1 << 7), (1 << 7) - 1)


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
