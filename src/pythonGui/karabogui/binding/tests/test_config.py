import numpy as np
from numpy.testing import assert_array_equal

from karabo.native import (
    Bool, Configurable, Hash, HashList, String, UInt8, VectorHash)

from ..config import validate_value, validate_vector_hash
from ..types import (
    BoolBinding, FloatBinding, Int8Binding, Uint8Binding, StringBinding,
    VectorDoubleBinding, VectorFloatBinding, VectorHashBinding,
    VectorInt8Binding, VectorUint8Binding, Int16Binding, Uint16Binding,
    Int32Binding, Uint32Binding, Int64Binding, Uint64Binding)


class TableRow(Configurable):
    stringProperty = String(defaultValue='foo')
    uintProperty = UInt8(defaultValue=1)
    boolProperty = Bool(defaultValue=True)


class TableRowEmpty(Configurable):
    stringProperty = String()
    uintProperty = UInt8()
    boolProperty = Bool()


def test_validate_value_float():
    """Test float binding validation"""
    # Check valid values
    binding = FloatBinding()
    assert_binding(binding, 1.3)
    assert_binding(binding, np.float32(1.3), expected=np.float32(1.3))
    assert_binding(binding, np.float64(1.3), expected=np.float64(1.3))
    assert_binding(binding, np.uint32(1), expected=float(1.0))
    assert_binding(binding, np.int32(-1), expected=float(-1.0))
    assert_binding(binding, "1.0", expected=float(1.0))

    # Check invalid values for floats
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)


def assert_binding(binding, value, expected=None, valid=True):
    """Validate a value against its binding

    :param binding: The corresponding binding
    :param value: The value to be verified
    :param expected: None by default. If not provided, the `value` is taken
                     as `expected` in the `valid` case.
    :param valid: If we have a `valid` value assert. Default is `True`.
    """
    validated = validate_value(binding, value)
    if valid and expected is None:
        expected = value
    assert validated == expected
    assert type(validated) == type(expected)


def test_validate_value_uint8():
    """Test uint8 binding validation."""
    binding = Uint8Binding()

    # Check valid values
    assert_binding(binding, 0, expected=np.uint8(0))
    assert_binding(binding, 255, expected=np.uint8(255))
    assert_binding(binding, np.int8(8), expected=np.uint8(8))
    assert_binding(binding, '1', expected=np.uint8(1))

    # Check invalid values
    assert_binding(binding, -1, valid=False)
    assert_binding(binding, 256, valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList(), valid=False)


def test_validate_value_int8():
    """Test int8 binding validation."""
    binding = Int8Binding()

    # Check valid values
    assert_binding(binding, -128, expected=np.int8(-128))
    assert_binding(binding, 127, expected=np.int8(127))
    assert_binding(binding, '1', expected=np.int8(1))
    assert_binding(binding, np.uint32(1), expected=np.int8(1))

    # Check invalid values
    assert_binding(binding, -129, valid=False)
    assert_binding(binding, 128, valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, np.float(1.0), valid=False)
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList([Hash(), Hash()]), valid=False)


def test_validate_value_int16():
    """Test the int16 binding validation."""
    binding = Int16Binding()

    # Check valid values
    assert_binding(binding, 0, expected=np.int16(0))
    assert_binding(binding, (2 ** 15) - 1, expected=np.int16((2 ** 15) - 1))
    assert_binding(binding, np.int8(8), expected=np.int16(8))
    assert_binding(binding, '1', expected=np.int16(1))

    # Check invalid values
    assert_binding(binding, -(2 ** 64), valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList(), valid=False)


def test_validate_value_uint16():
    """Test uint16 binding validation."""
    binding = Uint16Binding()

    # Check valid values
    assert_binding(binding, 0, expected=np.uint16(0))
    assert_binding(binding, (2 ** 16) - 1, expected=np.uint16((2 ** 16) - 1))
    assert_binding(binding, np.int8(8), expected=np.uint16(8))
    assert_binding(binding, '1', expected=np.uint16(1))

    # Check invalid values
    assert_binding(binding, -1, valid=False)
    assert_binding(binding, (2 ** 16), valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList(), valid=False)


def test_validate_value_int32():
    """Test int32 binding validation."""
    binding = Int32Binding()

    # Check valid values
    assert_binding(binding, -(2 ** 31), expected=np.int32(-(2 ** 31)))
    assert_binding(binding, (2 ** 31) - 1, expected=np.int32((2 ** 31) - 1))
    assert_binding(binding, '1', expected=np.int32(1))
    assert_binding(binding, np.uint32(1), expected=np.int32(1))

    # Check invalid values
    assert_binding(binding, -(2 ** 32), valid=False)
    assert_binding(binding, (2 ** 32) - 1, valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, np.float(1.0), valid=False)
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList([Hash(), Hash()]), valid=False)


def test_validate_value_uint32():
    """Test uint32 binding validation."""
    binding = Uint32Binding()

    # Check valid values
    assert_binding(binding, 1, expected=np.uint32(1))
    assert_binding(binding, (2 ** 32) - 1, expected=np.uint32((2 ** 32) - 1))
    assert_binding(binding, '1', expected=np.uint32(1))
    assert_binding(binding, np.uint32(1), expected=np.uint32(1))

    # Check invalid values
    assert_binding(binding, -(2 ** 32), valid=False)
    assert_binding(binding, (2 ** 32), valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, np.float(1.0), valid=False)
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList([Hash(), Hash()]), valid=False)


def test_validate_value_int64():
    """Test int64 binding validation."""
    binding = Int64Binding()

    # Check valid values
    assert_binding(binding, -(2 ** 63), expected=np.int64(-(2 ** 63)))
    assert_binding(binding, (2 ** 63) - 1, expected=np.int64((2 ** 63) - 1))
    assert_binding(binding, '1', expected=np.int64(1))
    assert_binding(binding, np.uint32(1), expected=np.int64(1))

    # Check invalid values
    assert_binding(binding, -(2 ** 64), valid=False)
    assert_binding(binding, (2 ** 64) - 1, valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, np.float(1.0), valid=False)
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList([Hash(), Hash()]), valid=False)


def test_validate_value_uint64():
    """Test uint64 binding validation."""
    binding = Uint64Binding()

    # Check valid values
    assert_binding(binding, 1, expected=np.uint64(1))
    assert_binding(binding, (2 ** 64) - 1, expected=np.uint64((2 ** 64) - 1))
    assert_binding(binding, '1', expected=np.uint64(1))
    assert_binding(binding, np.uint32(1), expected=np.uint64(1))

    # Check invalid values
    assert_binding(binding, -(2 ** 128), valid=False)
    assert_binding(binding, (2 ** 128) - 1, valid=False)
    assert_binding(binding, 1.0, valid=False)  # the value is a float
    assert_binding(binding, np.float(1.0), valid=False)
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList([Hash(), Hash()]), valid=False)


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


def test_validate_vector_hash():
    """Test vector hash binding validation.
    It returns both valid and invalid values in the form of a HashList([]) for
    each row."""
    binding = VectorHashBinding(row_schema=VectorHash(TableRow).rowSchema.hash)
    empty_binding = VectorHashBinding(
        row_schema=VectorHash(TableRowEmpty).rowSchema.hash)

    # Check with one valid hash
    valid_hash = Hash("stringProperty", "foo",
                      "uintProperty", 1,
                      "boolProperty", True)
    valid_hashlist = HashList([valid_hash])
    valid, invalid = validate_vector_hash(binding, valid_hashlist)
    assert valid == valid_hashlist
    assert invalid == HashList([None])

    # Check two valid hashes
    valid_hash = Hash("stringProperty", "foo",
                      "uintProperty", 1,
                      "boolProperty", True)
    valid_hashlist = HashList([valid_hash, valid_hash])
    valid, invalid = validate_vector_hash(binding, valid_hashlist)
    assert valid == valid_hashlist
    assert invalid == HashList([None, None])

    # Check with one hash that contains an invalid property
    invalid_hash = Hash("stringProperty", "foo",
                        "uintProperty", -1,  # invalid for a uintProperty
                        "boolProperty", True)
    valid, invalid = validate_vector_hash(binding, HashList([invalid_hash]))
    assert valid == HashList([None])
    assert invalid == HashList([invalid_hash])

    # Check with two hashes which one hash contains invalid properties
    valid_hash = Hash("stringProperty", "foo",
                      "uintProperty", 1,
                      "boolProperty", True)
    invalid_hash = Hash("stringProperty", "foo",
                        "uintProperty", -1,  # invalid for a uintProperty
                        "boolProperty", True)
    hash_list = HashList([valid_hash, invalid_hash])
    valid, invalid = validate_vector_hash(binding, hash_list)
    assert valid == HashList([valid_hash, None])  # drop the row with invalids
    assert invalid == HashList([None, invalid_hash])

    # Check with two hashes which both contains invalid properties
    valid_hash = Hash("stringProperty", [1, 2],  # invalid for a uintProperty
                      "uintProperty", 1,
                      "boolProperty", "bar")
    invalid_hash = Hash("stringProperty", "foo",
                        "uintProperty", -1,  # invalid for a uintProperty
                        "boolProperty", True)
    hash_list = HashList([valid_hash, invalid_hash])
    valid, invalid = validate_vector_hash(binding, hash_list)
    assert valid == HashList([None, None])  # drop the row with invalids
    assert invalid == HashList([valid_hash, invalid_hash])

    # Check with a hash that contains an invalid property and default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    valid, invalid = validate_vector_hash(binding, HashList([invalid_hash]))
    assert valid == HashList([Hash("stringProperty", "foo",
                                   "uintProperty", 1,  # default value
                                   "boolProperty", True)])
    assert invalid == HashList([None])

    # Check with a hash that contains an invalid property
    # and without default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    valid, invalid = validate_vector_hash(empty_binding,
                                          HashList([invalid_hash]))
    assert valid == HashList([None])
    assert invalid == HashList([Hash("stringProperty", "foo",
                                     "uintProperty", None,  # dropped value
                                     "boolProperty", True)])


def _assert_array(binding, value, dtype):
    array = validate_value(binding, value)
    assert_array_equal(array, value)
    assert array.dtype == dtype
