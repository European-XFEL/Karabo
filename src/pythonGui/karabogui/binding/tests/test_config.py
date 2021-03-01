import numpy as np
from numpy.testing import assert_array_equal

from karabo.common.const import KARABO_SCHEMA_DEFAULT_VALUE
from karabo.native import (
    AccessMode, Bool, Configurable, Hash, HashList, String, UInt8, VectorHash)

from karabogui.binding.config import (
    validate_value, validate_table_value, get_default_value,
    sanitize_table_value)
import karabogui.binding.types as types


class TableRow(Configurable):
    stringProperty = String(defaultValue='foo')
    uintProperty = UInt8(defaultValue=1)
    boolProperty = Bool(defaultValue=True)


class TableRowEmpty(Configurable):
    stringProperty = String()
    uintProperty = UInt8()
    boolProperty = Bool()


class TableRowReadOnly(Configurable):
    stringProperty = String(accessMode=AccessMode.READONLY)


class TableRowMixedDefaults(Configurable):
    stringProperty = String(defaultValue='bar')
    uintProperty = UInt8(defaultValue=20)
    boolProperty = Bool()


def test_validate_value_float():
    """Test float binding validation"""
    # Check valid values
    binding = types.FloatBinding()
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
    binding = types.Uint8Binding()

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
    binding = types.Int8Binding()

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
    binding = types.Int16Binding()

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
    binding = types.Uint16Binding()

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
    binding = types.Int32Binding()

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
    binding = types.Uint32Binding()

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
    binding = types.Int64Binding()

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
    binding = types.Uint64Binding()

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
    binding = types.StringBinding()

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
    binding = types.BoolBinding()

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
    binding = types.VectorUint8Binding()

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
    binding = types.VectorInt8Binding()

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
    binding = types.VectorFloatBinding()
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
    binding = types.VectorDoubleBinding()
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
    binding = types.VectorHashBinding(
        row_schema=VectorHash(TableRow).rowSchema.hash)
    empty_binding = types.VectorHashBinding(
        row_schema=VectorHash(TableRowEmpty).rowSchema.hash)

    # Check with one valid hash
    valid_hash = Hash("stringProperty", "foo",
                      "uintProperty", 1,
                      "boolProperty", True)
    valid_hashlist = HashList([valid_hash])
    valid, invalid = validate_table_value(binding, valid_hashlist)
    assert valid == valid_hashlist
    assert invalid == HashList([])

    # Check two valid hashes
    valid_hash = Hash("stringProperty", "foo",
                      "uintProperty", 1,
                      "boolProperty", True)
    valid_hashlist = HashList([valid_hash, valid_hash])
    valid, invalid = validate_table_value(binding, valid_hashlist)
    assert valid == valid_hashlist
    assert invalid == HashList([])

    # Check with one hash that contains an invalid property
    invalid_hash = Hash("stringProperty", "foo",
                        "uintProperty", -1,  # invalid for a uintProperty
                        "boolProperty", True)
    valid, invalid = validate_table_value(binding, HashList([invalid_hash]))
    assert valid == HashList([])
    assert invalid == HashList([invalid_hash])

    # Check with two hashes which one hash contains invalid properties
    valid_hash = Hash("stringProperty", "foo",
                      "uintProperty", 1,
                      "boolProperty", True)
    invalid_hash = Hash("stringProperty", "foo",
                        "uintProperty", -1,  # invalid for a uintProperty
                        "boolProperty", True)
    hash_list = HashList([valid_hash, invalid_hash])
    valid, invalid = validate_table_value(binding, hash_list)
    assert valid == HashList([valid_hash])  # drop the row with invalids
    assert invalid == HashList([invalid_hash])

    # Check with two hashes which both contains invalid properties
    valid_hash = Hash("stringProperty", [1, 2],  # invalid for a uintProperty
                      "uintProperty", 1,
                      "boolProperty", "bar")
    invalid_hash = Hash("stringProperty", "foo",
                        "uintProperty", -1,  # invalid for a uintProperty
                        "boolProperty", True)
    hash_list = HashList([valid_hash, invalid_hash])
    valid, invalid = validate_table_value(binding, hash_list)
    assert valid == HashList([])  # drop the row with invalids
    assert invalid == HashList([valid_hash, invalid_hash])

    # Check with a hash that contains an invalid property and default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    valid, invalid = validate_table_value(binding, HashList([invalid_hash]))
    assert valid == HashList([Hash("stringProperty", "foo",
                                   "uintProperty", 1,  # default value
                                   "boolProperty", True)])
    assert invalid == HashList([])

    # Check with a hash that contains an invalid property
    # and without default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    valid, invalid = validate_table_value(empty_binding,
                                          HashList([invalid_hash]))
    assert valid == HashList([])
    assert invalid == HashList([Hash("stringProperty", "foo",
                                     "uintProperty", None,  # dropped value
                                     "boolProperty", True)])


def test_sanitize_table():
    """Test vector hash binding sanitization."""
    binding = types.VectorHashBinding(
        row_schema=VectorHash(TableRow).rowSchema.hash)
    empty_binding = types.VectorHashBinding(
        row_schema=VectorHash(TableRowEmpty).rowSchema.hash)
    readonly_binding = types.VectorHashBinding(
        row_schema=VectorHash(TableRowReadOnly).rowSchema.hash)
    mixed_default_binding = types.VectorHashBinding(
        row_schema=VectorHash(TableRowMixedDefaults).rowSchema.hash)

    VALID_TABLE_HASH = Hash(
        "stringProperty", "foo",
        "uintProperty", 1,
        "boolProperty", True)

    # Check with a single hash conform to the schema
    hash_list = HashList([VALID_TABLE_HASH])
    value = sanitize_table_value(binding, hash_list)
    assert value == hash_list

    hash_list = HashList([VALID_TABLE_HASH, VALID_TABLE_HASH])
    value = sanitize_table_value(binding, hash_list)
    assert value == hash_list

    # Check one hash that contains an invalid value for a property
    invalid_hash = Hash(
        "stringProperty", "foo",
        "uintProperty", -1,  # invalid for a uintProperty
        "boolProperty", True)

    value = sanitize_table_value(binding, HashList([invalid_hash]))
    assert value == HashList([VALID_TABLE_HASH])

    # Check with two hashes which one hash contains invalid properties
    invalid_hash = Hash(
        "stringProperty", "foo",
        "uintProperty", -1,  # invalid for a uintProperty
        "boolProperty", True)
    hash_list = HashList([VALID_TABLE_HASH, invalid_hash])
    value = sanitize_table_value(binding, hash_list)
    assert value == HashList([VALID_TABLE_HASH, VALID_TABLE_HASH])

    # Check with two hashes which both contains invalid properties
    invalid_hash_1 = Hash(
        "stringProperty", [1, 2],  # invalid for a uintProperty
        "uintProperty", 1,
        "boolProperty", "bar")
    invalid_hash_2 = Hash(
        "stringProperty", "foo",
        "uintProperty", -1,  # invalid for a uintProperty
        "boolProperty", True)
    hash_list = HashList([invalid_hash_1, invalid_hash_2])
    value = sanitize_table_value(binding, hash_list)
    assert value == HashList([VALID_TABLE_HASH, VALID_TABLE_HASH])

    # Check with a hash that misses a property. The schema has a default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    value = sanitize_table_value(binding, HashList([invalid_hash]))
    assert value == HashList([VALID_TABLE_HASH])

    # Check with a hash that has false property keys.
    # The schema has a default values in this test case
    invalid_hash = Hash("stringProperty", "foo",
                        "uintPropertyWRONG", 1,
                        "boolPropertyWRONG", True)
    value = sanitize_table_value(binding, HashList([invalid_hash]))
    assert value == HashList([VALID_TABLE_HASH])

    # Check with a hash misses a property and the schema does not have
    # a default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    value = sanitize_table_value(empty_binding, HashList([invalid_hash]))
    assert value == HashList([Hash("stringProperty", "foo",
                                   "uintProperty", 0,  # forced value
                                   "boolProperty", True)])

    valid_hash = Hash("stringProperty", "foobar")
    value = sanitize_table_value(readonly_binding, HashList([valid_hash]))
    assert value == HashList([Hash("stringProperty", "foobar")])

    # Mixed table. We provide:
    # uintProperty with string, validation fails -> available default
    # missing string property, available default
    # missing bool property, no available default, forced
    valid_hash = Hash("uintProperty", "K")
    value = sanitize_table_value(mixed_default_binding, HashList([valid_hash]))
    assert value == HashList([Hash("stringProperty", "bar",
                                   "uintProperty", 20,
                                   "boolProperty", False)])


def _assert_array(binding, value, dtype):
    array = validate_value(binding, value)
    assert_array_equal(array, value)
    assert array.dtype == dtype


def test_default_value():
    """Test the default value generation of a binding"""
    binding = types.FloatBinding()
    # No default value provided, no `force` as default (False), value must
    # be `None` if not present
    value = get_default_value(binding)
    assert value is None
    # Force a default value for a float binding
    value = get_default_value(binding, force=True)
    assert value == 0.0
    # Apply a default value and retrieve default value
    binding.attributes[KARABO_SCHEMA_DEFAULT_VALUE] = 5.1
    value = get_default_value(binding, force=True)
    assert value == 5.1

    # Go through all other bindings
    binding = types.Int8Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Int16Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Int32Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Int64Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Uint8Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Uint16Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Uint32Binding()
    value = get_default_value(binding, force=True)
    assert value == 0
    binding = types.Uint64Binding()
    value = get_default_value(binding, force=True)
    assert value == 0

    binding = types.VectorHashBinding()
    value = get_default_value(binding, force=True)
    assert value == []

    binding = types.VectorInt8Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorInt16Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorInt32Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorInt64Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorUint8Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorUint16Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorUint32Binding()
    value = get_default_value(binding, force=True)
    assert value == []
    binding = types.VectorUint64Binding()
    value = get_default_value(binding, force=True)
    assert value == []

    binding = types.StringBinding()
    value = get_default_value(binding, force=True)
    assert value == ""

    binding = types.BoolBinding()
    value = get_default_value(binding, force=True)
    assert value is False

    binding = types.CharBinding()
    value = get_default_value(binding, force=True)
    assert value == ""

    binding = types.ByteArrayBinding()
    value = get_default_value(binding, force=True)
    assert value == bytearray([])

    binding = types.ComplexBinding()
    value = get_default_value(binding, force=True)
    assert value == 0.0
