import sys

import numpy as np

from karabo.common.api import (
    KARABO_SCHEMA_VALUE_TYPE, KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC,
    KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC, KARABO_SCHEMA_ABSOLUTE_ERROR,
    KARABO_SCHEMA_RELATIVE_ERROR
)
from karabo.middlelayer import Hash, Schema
from ..api import (
    BoolBinding, FloatBinding, Int8Binding, Int16Binding, Int32Binding,
    Int64Binding, Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding,
    VectorInt32Binding, fast_deepcopy, get_min_max, has_changes
)


def test_simple_int_min_max():
    assert get_min_max(Int8Binding()) == (-(1 << 7) + 1, (1 << 7) - 1)
    assert get_min_max(Int16Binding()) == (-(1 << 15) + 1, (1 << 15) - 1)
    assert get_min_max(Int32Binding()) == (-(1 << 31) + 1, (1 << 31) - 1)
    assert get_min_max(Int64Binding()) == (-(1 << 63) + 1, (1 << 63) - 1)

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


def test_fast_deepcopy():
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
    copy = fast_deepcopy(d)
    assert _safe_compare(copy, d)


def test_has_changes():
    binding = Int8Binding()
    assert has_changes(binding, None, 2)
    assert not has_changes(binding, 2, 2)

    binding = FloatBinding()
    assert not has_changes(binding, 2.0, 2.00005)
    assert has_changes(binding, 2.0, 2.0005)

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
