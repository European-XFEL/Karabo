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

import karabogui.binding.binding_types as types
from karabo.common.const import (
    KARABO_SCHEMA_DEFAULT_VALUE, KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC,
    KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC,
    KARABO_SCHEMA_MIN_SIZE, KARABO_SCHEMA_OPTIONS)
from karabo.native import (
    AccessMode, Bool, Configurable, Hash, HashList, String, UInt8)
from karabogui.binding.builder import build_binding
from karabogui.binding.validate import (
    convert_string, get_default_value, sanitize_table_value,
    validate_binding_configuration, validate_table_value, validate_value)
from karabogui.testing import get_simple_props_schema


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


class TableRowReadOnlyDefault(Configurable):
    uintProperty = UInt8(defaultValue=57,
                         accessMode=AccessMode.READONLY)


class TableRowMixedDefaults(Configurable):
    stringProperty = String(defaultValue='bar')
    uintProperty = UInt8(defaultValue=20)
    boolProperty = Bool()


def test_validate_value_float():
    """Test float binding validation"""
    # Check valid values
    binding = types.FloatBinding()
    assert_binding(binding, 1.3, expected=1.3)
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
    :param expected: Expected value after validation. Must be provided
                     on `valid` case.
    :param valid: If we have a `valid` value assert. Default is `True`.
    """
    validated = validate_value(binding, value)
    if not valid:
        assert validated is None
    else:
        # Always force to specify expected
        assert expected is not None
        assert validated == expected
        assert issubclass(type(validated), type(expected))


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
    assert_binding(binding, np.float64(1.0), valid=False)
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
    assert_binding(binding, np.float64(1.0), valid=False)
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
    assert_binding(binding, np.float64(1.0), valid=False)
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
    assert_binding(binding, np.float64(1.0), valid=False)
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
    assert_binding(binding, np.float64(1.0), valid=False)
    assert_binding(binding, "foo", valid=False)
    assert_binding(binding, [1, 2, 3], valid=False)
    assert_binding(binding, HashList([Hash(), Hash()]), valid=False)


def test_validate_min_max_binding():
    """Test the validate with min and max attributes of a binding"""
    attrs = {KARABO_SCHEMA_MIN_EXC: 0, KARABO_SCHEMA_MAX_EXC: 4}
    binding = types.Int8Binding(attributes=attrs)
    assert_binding(binding, 0, valid=False)
    assert_binding(binding, -1, valid=False)
    assert_binding(binding, 3, expected=np.int8(3))
    assert_binding(binding, '1', expected=np.int8(1))
    assert_binding(binding, 4, valid=False)

    attrs = {KARABO_SCHEMA_MIN_INC: 0, KARABO_SCHEMA_MAX_INC: 4}
    binding = types.Int8Binding(attributes=attrs)
    assert_binding(binding, 0, expected=np.int8(0))
    assert_binding(binding, 4, expected=np.int8(4))
    assert_binding(binding, '1', expected=np.int8(1))

    # And the float
    attrs = {KARABO_SCHEMA_MIN_EXC: 0, KARABO_SCHEMA_MAX_EXC: 4}
    binding = types.FloatBinding(attributes=attrs)
    assert_binding(binding, 0.0, valid=False)
    assert_binding(binding, 0.00000000000001, expected=float(0.00000000000001))
    assert_binding(binding, -4.7, valid=False)
    assert_binding(binding, 3.99999999999999, expected=float(3.99999999999999))
    assert_binding(binding, 4.00000000000000000000000000000001, valid=False)

    attrs = {KARABO_SCHEMA_MIN_INC: 0, KARABO_SCHEMA_MAX_INC: 4}
    binding = types.FloatBinding(attributes=attrs)
    assert_binding(binding, 0, expected=float(0.0))
    assert_binding(binding, 4, expected=float(4.0))
    assert_binding(binding, '1', expected=float(1.0))


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


def test_validate_value_vector_size():
    attributes = {KARABO_SCHEMA_MIN_SIZE: 3, KARABO_SCHEMA_MAX_SIZE: 5}
    binding = types.VectorDoubleBinding(attributes=attributes)
    array = np.array([-1, 0, 1])
    _assert_array(binding, array, dtype=np.float64)
    array = np.array([-1, 0, 1, 25, 100])
    _assert_array(binding, array, dtype=np.float64)

    # Check valid values, minimum size
    array = np.array([-1, 0])
    assert validate_value(binding, array) is None

    # Check valid values, maximum size
    array = np.array([-1, 0, 1, 25, 100, 129])
    assert validate_value(binding, array) is None


def test_validate_value_options():
    attributes = {KARABO_SCHEMA_OPTIONS: [3, 4, 5]}
    binding = types.Uint8Binding(attributes=attributes)
    assert validate_value(binding, "3") == 3
    assert validate_value(binding, 4) == 4
    # Not in options!
    assert validate_value(binding, 2) is None
    assert validate_value(binding, 6) is None

    attributes = {KARABO_SCHEMA_OPTIONS: ["ham", "eggs"]}
    binding = types.StringBinding(attributes=attributes)
    assert validate_value(binding, "ham") == "ham"
    assert validate_value(binding, "eggs") == "eggs"
    assert validate_value(binding, 2) is None
    assert validate_value(binding, "XFEL") is None


def test_validate_vector_hash():
    """Test vector hash binding validation.
    It returns both valid and invalid values in the form of a HashList([]) for
    each row."""

    binding = types.VectorHashBinding(
        row_schema=TableRow.getClassSchema().hash)
    empty_binding = types.VectorHashBinding(
        row_schema=TableRowEmpty.getClassSchema().hash)

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
                                     "uintProperty", None,  # dropped val
                                     "boolProperty", True)])

    # ReadOnly tables should be still validated
    # -----------------------------------------------------------------------

    # 1. A fully valid Hash
    readonly_binding = types.VectorHashBinding(
        row_schema=TableRowReadOnly.getClassSchema().hash)
    readonly_binding_default = types.VectorHashBinding(
        row_schema=TableRowReadOnlyDefault.getClassSchema().hash)

    valid_hash = Hash("stringProperty", "karabo")
    valid, invalid = validate_table_value(readonly_binding,
                                          HashList([valid_hash]))
    assert valid == HashList([Hash("stringProperty", "karabo")])
    assert invalid == HashList([])

    # 2. A valid string but boolean is not in the schema

    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)  # A value to many
    valid, invalid = validate_table_value(readonly_binding,
                                          HashList([invalid_hash]))
    assert valid == HashList([Hash("stringProperty", "foo")])
    assert invalid == HashList([])

    # 3. An empty Hash without default value in Schema
    invalid_hash = Hash()
    valid, invalid = validate_table_value(readonly_binding,
                                          HashList([invalid_hash]))
    assert valid == HashList([])
    assert invalid == HashList([Hash("stringProperty", None)])

    # 3. An empty Hash WITH default value in Schema
    invalid_hash = Hash()
    valid, invalid = validate_table_value(readonly_binding_default,
                                          HashList([invalid_hash]))
    assert valid == HashList([Hash("uintProperty", 57)])
    assert invalid == HashList([])


def test_sanitize_table():
    """Test vector hash binding sanitization."""
    binding = types.VectorHashBinding(
        row_schema=TableRow.getClassSchema().hash)
    empty_binding = types.VectorHashBinding(
        row_schema=TableRowEmpty.getClassSchema().hash)
    readonly_binding = types.VectorHashBinding(
        row_schema=TableRowReadOnly.getClassSchema().hash)
    mixed_default_binding = types.VectorHashBinding(
        row_schema=TableRowMixedDefaults.getClassSchema().hash)

    VALID_TABLE_HASH = Hash(
        "stringProperty", "foo",
        "uintProperty", 1,
        "boolProperty", True)

    # Check with a single hash conform to the schema
    hash_list = HashList([VALID_TABLE_HASH])
    success, value = sanitize_table_value(binding, hash_list)
    assert value == hash_list
    assert success is True

    hash_list = HashList([VALID_TABLE_HASH, VALID_TABLE_HASH])
    success, value = sanitize_table_value(binding, hash_list)
    assert value == hash_list
    assert success is True

    # Check one hash that contains an invalid value for a property
    invalid_hash = Hash(
        "stringProperty", "foo",
        "uintProperty", -1,  # invalid for a uintProperty
        "boolProperty", True)

    success, value = sanitize_table_value(binding, HashList([invalid_hash]))
    assert value == HashList([VALID_TABLE_HASH])
    assert success is False

    # Check with two hashes which one hash contains invalid properties
    invalid_hash = Hash(
        "stringProperty", "foo",
        "uintProperty", -1,  # invalid for a uintProperty
        "boolProperty", True)
    hash_list = HashList([VALID_TABLE_HASH, invalid_hash])
    success, value = sanitize_table_value(binding, hash_list)
    assert value == HashList([VALID_TABLE_HASH, VALID_TABLE_HASH])
    assert success is False

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
    success, value = sanitize_table_value(binding, hash_list)
    assert value == HashList([VALID_TABLE_HASH, VALID_TABLE_HASH])
    assert success is False

    # Check with a hash that misses a property. The schema has a default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    success, value = sanitize_table_value(binding, HashList([invalid_hash]))
    assert value == HashList([VALID_TABLE_HASH])
    assert success is False

    # Check with a hash that has false property keys.
    # The schema has a default values in this test case
    invalid_hash = Hash("stringProperty", "foo",
                        "uintPropertyWRONG", 1,
                        "boolPropertyWRONG", True)
    success, value = sanitize_table_value(binding, HashList([invalid_hash]))
    assert value == HashList([VALID_TABLE_HASH])
    assert success is False

    # Check with a hash misses a property and the schema does not have
    # a default value
    invalid_hash = Hash("stringProperty", "foo",
                        "boolProperty", True)
    success, value = sanitize_table_value(empty_binding,
                                          HashList([invalid_hash]))
    assert value == HashList([Hash("stringProperty", "foo",
                                   "uintProperty", 0,  # forced value
                                   "boolProperty", True)])
    assert success is False

    # Just one property, but no real validation
    valid_hash = Hash("stringProperty", "foobar")
    success, value = sanitize_table_value(readonly_binding,
                                          HashList([valid_hash]))
    assert value == HashList([Hash("stringProperty", "foobar")])
    assert success is True

    # Mixed table. We provide:
    # uintProperty with string, validation fails -> available default
    # missing string property, available default
    # missing bool property, no available default, forced
    valid_hash = Hash("uintProperty", "K")
    success, value = sanitize_table_value(mixed_default_binding,
                                          HashList([valid_hash]))
    assert value == HashList([Hash("stringProperty", "bar",
                                   "uintProperty", 20,
                                   "boolProperty", False)])
    assert success is False


def _assert_array(binding, value, dtype):
    array = validate_value(binding, value)
    np.testing.assert_array_equal(array, value)
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


def test_default_value_minimum():
    """Test the default value generation of a binding"""
    attributes = {KARABO_SCHEMA_MIN_INC: 2}
    binding = types.Int8Binding(attributes=attributes)
    # 1. No default value provided, we force to minimum
    value = get_default_value(binding, force=True)
    assert value == 2
    # 2. Provide a default, higher than minimum
    attributes = {KARABO_SCHEMA_MIN_INC: 2, KARABO_SCHEMA_DEFAULT_VALUE: 5}
    binding = types.Int8Binding(attributes=attributes)
    value = get_default_value(binding, force=True)
    assert value == 5
    # 3. Check options
    attributes = {KARABO_SCHEMA_OPTIONS: [5, 6, 7]}
    binding = types.Int8Binding(attributes=attributes)
    value = get_default_value(binding, force=True)
    assert value == 5

    # ------------------------------

    attributes = {KARABO_SCHEMA_MIN_EXC: 0.0}
    binding = types.FloatBinding(attributes=attributes)
    # 1. No default value provided, we force to slighly above minimum
    value = get_default_value(binding, force=True)
    assert value > 0.0
    assert value < 1e-32
    # 2. Provide a default, higher than minimum
    attributes = {KARABO_SCHEMA_MIN_INC: 2.1, KARABO_SCHEMA_DEFAULT_VALUE: 5.2}
    binding = types.FloatBinding(attributes=attributes)
    value = get_default_value(binding, force=True)
    assert value == 5.2
    # 3. Check options
    attributes = {KARABO_SCHEMA_OPTIONS: [5.1, 6.2, 7.3]}
    binding = types.FloatBinding(attributes=attributes)
    value = get_default_value(binding, force=True)
    assert value == 5.1


def test_default_value_vector_minsize():
    """Test the default value generation of a binding"""
    attributes = {KARABO_SCHEMA_MIN_SIZE: 2}
    binding = types.VectorUint8Binding(attributes=attributes)
    # 1. No default value provided, we force to minimum with minSize 2
    value = get_default_value(binding, force=True)
    assert isinstance(value, np.ndarray)
    assert (value == [0, 0]).all()
    assert value.dtype == np.uint8

    # 2. Provide a default, higher than min size
    attributes = {KARABO_SCHEMA_MIN_SIZE: 2,
                  KARABO_SCHEMA_DEFAULT_VALUE: [3, 3, 3]}
    binding = types.Int8Binding(attributes=attributes)
    value = get_default_value(binding, force=True)
    assert value == [3, 3, 3]
    # Default value in this test provided without numpy type
    assert getattr(value, 'dtype', None) is None

    # 3. No default value provided, vector string of size 4
    attributes = {KARABO_SCHEMA_MIN_SIZE: 4}
    binding = types.VectorStringBinding(attributes=attributes)
    value = get_default_value(binding, force=True)
    assert value == ["", "", "", ""]


def test_validate_binding_configuration():
    schema = get_simple_props_schema()
    binding = build_binding(schema)

    # Note: validate binding configuration uses `validate_value`, which is
    # extensively tested. We check that the iteration works

    # 1. Leaf element testing
    config = Hash("boolProperty", False)
    fails = validate_binding_configuration(binding, config)
    # Boolean is fine
    assert fails.empty()

    config = Hash("intProperty", 2)
    fails = validate_binding_configuration(binding, config)
    assert fails.empty()

    config = Hash("intProperty", 2.0)
    fails = validate_binding_configuration(binding, config)
    assert "intProperty" in fails

    config = Hash("NOTINSCHEMA", False)
    # Right now, properties that are not in the schema, are not in fails
    # This might change in the future
    fails = validate_binding_configuration(binding, config)
    # Boolean is fine
    assert fails.empty()

    # 2. Node element testing
    # Test that elements in nodes are validated
    config = Hash("node.foo", "XFEL")
    fails = validate_binding_configuration(binding, config)
    assert "node.foo" not in fails

    config = Hash("node.bar", np.array([]), "node.charlie", 2)
    fails = validate_binding_configuration(binding, config)
    assert "node.bar" in fails
    assert "node.charlie" not in fails

    # 3. Table element testing
    # A valid config
    config = Hash("table", [Hash("foo", False, "bar", "XFEL", "charlie", 2),
                            Hash("foo", True, "bar", "laser", "charlie", 0)])
    fails = validate_binding_configuration(binding, config)
    assert "table" not in fails

    # Invalid, charlie becomes a float although integer
    config = Hash("table", [Hash("foo", False, "bar", "XFEL", "charlie", 3.0)])
    fails = validate_binding_configuration(binding, config)
    assert "table" in fails

    # Put None table
    config = Hash("table", None)
    fails = validate_binding_configuration(binding, config)
    assert "table" in fails


def test_convert_string():
    value, success = convert_string("2")
    assert value == 2
    assert success

    value, success = convert_string("00001")
    assert value == "00001"
    assert not success

    value, success = convert_string("1.3")
    assert value == 1.3
    assert success
