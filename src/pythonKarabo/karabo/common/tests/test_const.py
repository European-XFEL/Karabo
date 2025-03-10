# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from inspect import isfunction

from karabo.common import const as constmod


def test_schema_attributes_def():
    tuple_symbol = "KARABO_SCHEMA_ATTRIBUTES"
    ignored_symbols = (
        "KARABO_SCHEMA_DEFAULT_SCENE",
        "KARABO_LOGGER_CONTENT_DEFAULT",
        "KARABO_CLASS_ID_STATE",
        "KARABO_CLASS_ID_ALARM"
    )

    all_symbols = dir(constmod)
    all_symbols = [
        s
        for s in all_symbols
        if not s.startswith("__") and not s.startswith("KARABO_TYPE")
    ]
    # Remove all functions
    all_symbols = [
        s for s in all_symbols if not isfunction(getattr(constmod, s))
    ]
    all_symbols.remove(tuple_symbol)
    for symbol in ignored_symbols:
        all_symbols.remove(symbol)
    all_values = {getattr(constmod, s) for s in all_symbols}

    # Make sure that all exported attribute names make it into the
    # KARABO_SCHEMA_ATTRIBUTES tuple.
    # NOTE: If this test fails, go to const.py and add entries to
    # KARABO_SCHEMA_ATTRIBUTES until this test passes
    assert all_values == set(getattr(constmod, tuple_symbol))


def test_schema_types():
    """Make sure that all the schema types appear in the tuple summary"""
    tuple_symbol = "KARABO_TYPES"
    all_symbols = dir(constmod)
    all_symbols = [s for s in all_symbols if s.startswith("KARABO_TYPE")]
    all_symbols.remove(tuple_symbol)

    all_values = {getattr(constmod, s) for s in all_symbols}
    assert all_values == set(getattr(constmod, tuple_symbol))


def test_karabotype_checks():
    """Test the check of karabo types"""
    assert constmod.is_float_type("FLOAT")
    assert constmod.is_float_type("DOUBLE")
    assert constmod.is_float_type("COMPLEX_FLOAT")
    assert constmod.is_float_type("COMPLEX_DOUBLE")

    assert not constmod.is_float_type("BOOL")
    assert not constmod.is_float_type("ABOOL")

    assert constmod.is_integer_type("INT8")
    assert constmod.is_integer_type("INT16")
    assert constmod.is_integer_type("INT32")
    assert constmod.is_integer_type("INT64")
    assert constmod.is_integer_type("UINT8")
    assert constmod.is_integer_type("UINT16")
    assert constmod.is_integer_type("UINT32")
    assert constmod.is_integer_type("UINT64")
    assert not constmod.is_integer_type("UINT128")
    assert not constmod.is_integer_type("VECTOR_FLOAT")

    assert constmod.is_signed_integer_type("INT8")
    assert constmod.is_signed_integer_type("INT16")
    assert constmod.is_signed_integer_type("INT32")
    assert constmod.is_signed_integer_type("INT64")
    assert not constmod.is_signed_integer_type("UINT8")

    assert constmod.is_unsigned_integer_type("UINT8")
    assert constmod.is_unsigned_integer_type("UINT16")
    assert constmod.is_unsigned_integer_type("UINT32")
    assert constmod.is_unsigned_integer_type("UINT64")
    assert not constmod.is_unsigned_integer_type("INT8")

    assert constmod.is_boolean_type("BOOL")
    assert not constmod.is_boolean_type("VECTOR_BOOL")
    assert not constmod.is_boolean_type("INT8")

    assert constmod.is_string_type("STRING")
    assert not constmod.is_string_type("CHAR")

    assert constmod.is_string_like_type("STRING")
    assert constmod.is_string_like_type("CHAR")

    assert constmod.is_bytearray_type("BYTE_ARRAY")
    assert not constmod.is_bytearray_type("CHAR")
    assert not constmod.is_bytearray_type("BYTEARRAY")

    assert constmod.is_vector_bool_type("VECTOR_BOOL")
    assert not constmod.is_vector_bool_type("VECTOR_UINT8")

    assert constmod.is_vector_char_type("VECTOR_CHAR")
    assert not constmod.is_vector_bool_type("VECTOR_UINT8")
    assert not constmod.is_vector_bool_type("VECTOR_STRING")

    assert constmod.is_vector_string_type("VECTOR_STRING")
    assert not constmod.is_vector_string_type("VECTOR_CHAR")
    assert constmod.is_vector_type("VECTOR_DOUBLE")
    assert constmod.is_vector_type("VECTOR_UINT8")
    assert constmod.is_vector_type("VECTOR_STRING")
    assert constmod.is_vector_type("VECTOR_HASH")
    assert constmod.is_vector_type("VECTOR_COMPLEX_FLOAT")
    assert constmod.is_vector_type("VECTOR_COMPLEX_DOUBLE")
    assert constmod.is_vector_type("VECTOR_BOOL")
    assert constmod.is_vector_type("VECTOR_INT8")
    assert constmod.is_vector_type("VECTOR_INT16")
    assert constmod.is_vector_type("VECTOR_INT32")
    assert constmod.is_vector_type("VECTOR_INT64")
    assert constmod.is_vector_type("VECTOR_UINT8")
    assert constmod.is_vector_type("VECTOR_UINT16")
    assert constmod.is_vector_type("VECTOR_UINT32")
    assert constmod.is_vector_type("VECTOR_UINT64")
    assert not constmod.is_vector_type("BOOL")
    assert not constmod.is_vector_type("VECTOR_CHAR")

    assert constmod.is_vector_integer_type("VECTOR_INT8")
    assert constmod.is_vector_integer_type("VECTOR_INT16")
    assert constmod.is_vector_integer_type("VECTOR_INT32")
    assert constmod.is_vector_integer_type("VECTOR_INT64")
    assert constmod.is_vector_integer_type("VECTOR_UINT8")
    assert constmod.is_vector_integer_type("VECTOR_UINT16")
    assert constmod.is_vector_integer_type("VECTOR_UINT32")
    assert constmod.is_vector_integer_type("VECTOR_UINT64")
    assert not constmod.is_vector_integer_type("VECTOR_FLOAT")
    assert not constmod.is_vector_integer_type("VECTOR_DOUBLE")

    assert constmod.is_vector_float_type("VECTOR_FLOAT")
    assert constmod.is_vector_float_type("VECTOR_DOUBLE")
    assert constmod.is_vector_float_type("VECTOR_COMPLEX_FLOAT")
    assert constmod.is_vector_float_type("VECTOR_COMPLEX_DOUBLE")
    assert not constmod.is_vector_float_type("VECTOR_UINT64")

    assert constmod.is_vector_hash_type("VECTOR_HASH")
    assert not constmod.is_vector_hash_type("VECTOR_BOOL")

    assert constmod.is_vector_bool_type("VECTOR_BOOL")
    assert not constmod.is_vector_bool_type("VECTOR_HASH")
