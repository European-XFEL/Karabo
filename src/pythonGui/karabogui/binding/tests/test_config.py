import numpy as np
from numpy.testing import assert_array_equal

from karabo.native import (
    Bool, Configurable, Hash, HashList, String, UInt8)

from ..config import validate_value
from ..types import (
    BoolBinding, FloatBinding, Int8Binding, Uint8Binding, StringBinding,
    VectorDoubleBinding, VectorFloatBinding, VectorInt8Binding,
    VectorUint8Binding)


class TableRow(Configurable):
    stringProperty = String(defaultValue='foo')
    uintProperty = UInt8(defaultValue=1)
    boolProperty = Bool(defaultValue=True)


def test_validate_value_float():
    """Test float binding validation.
    If valid, it should return the casted value"""
    binding = FloatBinding()

    # Check valid values
    assert validate_value(binding, 1.3) == 1.3
    assert validate_value(binding, np.float32(1.3)) == np.float32(1.3)
    assert validate_value(binding, np.float64(1.3)) == np.float64(1.3)
    assert validate_value(binding, np.uint32(1)) == np.float32(1.0)
    assert validate_value(binding, np.int32(-1)) == np.float32(-1.0)
    assert validate_value(binding, "1.0") == np.float32(1.0)

    # Check invalid values for floats
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, [1, 2, 3]) is None


def test_validate_value_uint8():
    """Test uint8 binding validation.
    If valid, it should return the casted value"""
    binding = Uint8Binding()

    # Check valid values
    assert validate_value(binding, 0) == 0
    assert validate_value(binding, 255) == 255
    assert validate_value(binding, '1') == 1

    # Check invalid values
    assert validate_value(binding, -1) is None
    assert validate_value(binding, 256) is None
    assert validate_value(binding, 1.0) is None  # the value is a float
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, [1, 2, 3]) is None
    assert validate_value(binding, HashList()) is None


def test_validate_value_int8():
    """Test int8 binding validation.
    If valid, it should return the casted value"""
    binding = Int8Binding()

    # Check valid values
    assert validate_value(binding, -128) == -128
    assert validate_value(binding, 127) == 127
    assert validate_value(binding, '1') == 1

    # Check invalid values
    assert validate_value(binding, -129) is None
    assert validate_value(binding, 128) is None
    assert validate_value(binding, 1.0) is None  # the value is a float
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, [1, 2, 3]) is None
    assert validate_value(binding, HashList([Hash(), Hash()])) is None


def test_validate_value_string():
    """Test string binding validation.
    If valid, it should return the casted value"""
    binding = StringBinding()

    # Check valid values
    assert validate_value(binding, "1") == "1"
    assert validate_value(binding, 1) == "1"
    assert validate_value(binding, 1.0) == "1.0"

    # Check invalid values
    assert validate_value(binding, [1, 2, 3]) is None
    assert validate_value(binding, HashList()) is None


def test_validate_value_bool():
    """Test bool binding validation.
    If valid, it should return the casted value. The casting considers the
    the __bool__ of the object (the conventional way), not sure if it's what
    we want in Karabo."""
    binding = BoolBinding()

    # Check valid values
    assert validate_value(binding, 1) is True
    assert validate_value(binding, 0) is False
    assert validate_value(binding, 1.0) is True
    assert validate_value(binding, 0) is False
    assert validate_value(binding, '1') is True
    assert validate_value(binding, '0') is False
    assert validate_value(binding, "True") is True
    assert validate_value(binding, "False") is False

    # Check dubious values
    assert validate_value(binding, [1, 2, 3]) is True  # this should be invalid
    assert validate_value(binding, 2.0) is True  # this should be invalid
    assert validate_value(binding, HashList([Hash(), Hash()])) is True


def test_validate_value_vectoruint8():
    binding = VectorUint8Binding()

    array = np.array([0, 10, 255])
    # Check valid values
    _assert_array(binding, array, dtype=np.uint8)
    _assert_array(binding, array.astype(np.float32), dtype=np.uint8)
    _assert_array(binding, array.astype(np.uint32), dtype=np.uint8)

    # Check invalid values
    assert validate_value(binding, [-1]) is None
    assert validate_value(binding, [256]) is None
    assert validate_value(binding, 0) is None
    assert validate_value(binding, 255) is None
    assert validate_value(binding, 1.0) is None
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, HashList([Hash(), Hash()])) is None


def test_validate_value_vectorint8():
    binding = VectorInt8Binding()

    # Check valid values
    _assert_array(binding, [-1, 0, 1], dtype=np.int8)
    _assert_array(binding, [-128, 0, 127], dtype=np.int8)
    _assert_array(binding, [-1.0, 0.0, 1.0], dtype=np.int8)

    # Check invalid values
    assert validate_value(binding, [129]) is None
    assert validate_value(binding, [128]) is None
    assert validate_value(binding, -128) is None
    assert validate_value(binding, 127) is None
    assert validate_value(binding, 1.0) is None
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, HashList([Hash(), Hash()])) is None


def test_validate_value_vectorfloat():
    binding = VectorFloatBinding()
    array = np.array([-1, 0, 1])

    # Check valid values
    _assert_array(binding, array, dtype=np.float32)
    _assert_array(binding, array.astype(np.float32), dtype=np.float32)
    _assert_array(binding, array.astype(np.float64), dtype=np.float32)

    # Check invalid values
    assert validate_value(binding, -129) is None
    assert validate_value(binding, 128) is None
    assert validate_value(binding, 1.0) is None
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, HashList([Hash(), Hash()])) is None


def test_validate_value_vectordouble():
    binding = VectorDoubleBinding()
    array = np.array([-1, 0, 1])

    # Check valid values
    _assert_array(binding, array, dtype=np.float64)
    _assert_array(binding, array.astype(np.float32), dtype=np.float64)
    _assert_array(binding, array.astype(np.float64), dtype=np.float64)

    # Check invalid values
    assert validate_value(binding, -129) is None
    assert validate_value(binding, 128) is None
    assert validate_value(binding, 1.0) is None
    assert validate_value(binding, "foo") is None
    assert validate_value(binding, HashList([Hash(), Hash()])) is None


def _assert_array(binding, value, dtype):
    array = validate_value(binding, value)
    assert_array_equal(array, value)
    assert array.dtype == dtype
