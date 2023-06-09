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
from unittest import TestCase, main

import numpy as np

from karabo.native import (
    AccessMode, Bool, ByteArray, Char, Configurable, Double, Float, Int8,
    Int32, RegexString, String, TypeHash, TypeNone, TypeSchema, VectorChar,
    VectorDouble, VectorHash, VectorRegexString, VectorString, VectorUInt8,
    get_default_value, sanitize_table_schema)


def get_test_table(access=AccessMode.READONLY):
    class TestTable(Configurable):
        """A Configurable with a Table Element."""
        table = VectorHash(
            rows=Row,
            displayedName="Table",
            accessMode=access)

    return TestTable


class Row(Configurable):
    integerPropertyNotValid = Int32(accessMode=AccessMode.READONLY)
    stringPropertyNotValid = String()
    floatPropertyNotValid = Float(accessMode=AccessMode.INITONLY)
    doublePropertyNotValid = Double()
    vectorPropertyNotValid = VectorDouble()
    boolPropertyNotValid = Bool()

    integerProperty = Int32(defaultValue=2)
    stringProperty = String(defaultValue="exfl")
    floatProperty = Float(defaultValue=1.0)
    doubleProperty = Double(defaultValue=2.0)
    vectorProperty = VectorDouble(defaultValue=[1.2, 1.1])
    boolProperty = Bool(defaultValue=True)


class Tests(TestCase):

    def test_sanitize_table(self):
        table_row = Row.getClassSchema()
        table_hash = table_row.hash

        # 1. Check first that noDefaults have no default value in row schema
        no_defaults = [
            "integerPropertyNotValid",
            "stringPropertyNotValid",
            "floatPropertyNotValid",
            "doublePropertyNotValid",
            "vectorPropertyNotValid",
            "boolPropertyNotValid"]

        for prop in no_defaults:
            attrs = table_hash[prop, ...]
            self.assertNotIn("defaultValue", attrs)

        # 1.1. Check accessMode
        access = table_hash["floatPropertyNotValid", "accessMode"]
        self.assertEqual(access, AccessMode.INITONLY.value)

        # Check that defaults have a default value in the row schema
        defaults = {
            "integerProperty": 2,
            "stringProperty": "exfl",
            "floatProperty": 1.0,
            "doubleProperty": 2.0,
            "vectorProperty": [1.2, 1.1],
            "boolProperty": True}

        for prop, value in defaults.items():
            attrs = table_hash[prop, ...]
            self.assertEqual(attrs["defaultValue"], value)

        # In place modification, readonly false
        sanitize_table_schema(table_row, False)
        valid_table_hash = table_row.hash
        new_defaults = {
            "integerPropertyNotValid": 0,
            "stringPropertyNotValid": "",
            "floatPropertyNotValid": 0.0,
            "doublePropertyNotValid": 0.0,
            "vectorPropertyNotValid": [],
            "boolPropertyNotValid": False,
        }
        for prop, value in new_defaults.items():
            attrs = valid_table_hash[prop, ...]
            self.assertEqual(attrs["defaultValue"], value)

        # 2.1. Check accessMode sanitize
        access = table_hash["floatPropertyNotValid", "accessMode"]
        self.assertEqual(access, AccessMode.RECONFIGURABLE.value)

    def test_table_configurable_readonly(self):
        """Test that a readOnly table does not get defaults"""

        # 1 reconfigurable
        schema = get_test_table().getClassSchema()
        table_hash = schema.hash["table", "rowSchema"].hash
        no_defaults = [
            "integerPropertyNotValid",
            "stringPropertyNotValid",
            "floatPropertyNotValid",
            "doublePropertyNotValid",
            "vectorPropertyNotValid",
            "boolPropertyNotValid"]

        for prop in no_defaults:
            attrs = table_hash[prop, ...]
            self.assertNotIn("defaultValue", attrs)

        # Now we can test that a reconfigurable table
        # is sanitized with defaults!
        schema = get_test_table(
            access=AccessMode.RECONFIGURABLE).getClassSchema()
        table_hash = schema.hash["table", "rowSchema"].hash
        for prop in no_defaults:
            attrs = table_hash[prop, ...]
            self.assertIn("defaultValue", attrs)

    def test_not_supported_sanitize(self):

        def get_table(desc):
            class Row(Configurable):
                notValid = desc()

            return Row

        for descriptor in [ByteArray, Char, VectorChar, RegexString,
                           TypeHash, TypeNone, TypeSchema, VectorRegexString]:
            with self.assertRaises(TypeError):
                table_row = get_table(descriptor).getClassSchema()
                # Use readOnly here to validate only the descriptor
                sanitize_table_schema(table_row, True)

    def test_default_value(self):
        integerPropertyNoDefault = Int32(accessMode=AccessMode.READONLY)

        default = get_default_value(integerPropertyNoDefault)
        self.assertEqual(default, None)
        default = get_default_value(integerPropertyNoDefault, force=True)
        self.assertEqual(default, 0)

        stringPropertyNoDefault = String()
        default = get_default_value(stringPropertyNoDefault, force=True)
        self.assertEqual(default, "")

        floatPropertyNoDefault = Float(accessMode=AccessMode.INITONLY)
        default = get_default_value(floatPropertyNoDefault, force=True)
        self.assertEqual(default, 0.0)

        doublePropertyNoDefault = Double()
        default = get_default_value(doublePropertyNoDefault, force=True)
        self.assertEqual(default, 0.0)

        vectorPropertyNoDefault = VectorDouble()
        default = get_default_value(vectorPropertyNoDefault, force=True)
        self.assertEqual(default, [])

        integerProperty = Int32(defaultValue=2)
        default = get_default_value(integerProperty)
        self.assertEqual(default, 2)

        stringProperty = String(defaultValue="exfl")
        default = get_default_value(stringProperty)
        self.assertEqual(default, "exfl")

        floatProperty = Float(defaultValue=1.0)
        default = get_default_value(floatProperty)
        self.assertEqual(default, 1.0)

        doubleProperty = Double(defaultValue=2.0)
        default = get_default_value(doubleProperty)
        self.assertEqual(default, 2.0)

        vectorProperty = VectorDouble(defaultValue=[1.2, 1.1])
        default = get_default_value(vectorProperty)
        self.assertEqual(default, [1.2, 1.1])

        boolProperty = Bool(defaultValue=True)
        default = get_default_value(boolProperty)
        self.assertEqual(default, True)

    def test_default_value_vector_minsize(self):
        """Test the default value generation"""
        vectorProperty = VectorDouble(minSize=2)
        # 1. No default value provided, we force to minimum with minSize 2
        value = get_default_value(vectorProperty, force=True)
        assert isinstance(value, np.ndarray)
        assert (value == [0, 0]).all()
        assert value.dtype == np.float64

        # 2. Provide a default, higher than min size
        vectorProperty = VectorUInt8(minSize=2, defaultValue=[3, 3, 3])
        value = get_default_value(vectorProperty, force=True)
        assert value == [3, 3, 3]
        # Default value in this test provided without numpy type
        assert getattr(value, 'dtype', None) is None

        # 3. No default value provided, vector string of size 4
        vectorProperty = VectorString(minSize=4)
        value = get_default_value(vectorProperty, force=True)
        assert value == ["", "", "", ""]

    def test_default_value_minimum(self):
        """Test the default value generation of a simple descriptor"""
        intProperty = Int8(minInc=2)
        # 1. No default value provided, we force to minimum
        value = get_default_value(intProperty, force=True)
        assert value == 2
        assert value.dtype == np.int8
        # 2. Provide a default, higher than minimum
        intProperty = Int8(minInc=2, defaultValue=5)
        value = get_default_value(intProperty, force=True)
        assert value == 5
        assert value.dtype == np.int8
        # 3. Check options
        intProperty = Int8(options=[5, 6, 7])
        value = get_default_value(intProperty, force=True)
        assert value == 5
        # 4. No default value provided, we force to default, but 0
        intProperty = Int8(minInc=-2)
        value = get_default_value(intProperty, force=True)
        assert value == 0

        # ------------------------------

        floatProperty = Float(minExc=0)
        # 1. No default value provided, we force to slighly above minimum
        value = get_default_value(floatProperty, force=True)
        assert value > 0.0
        assert value < 1e-32
        # 2. Provide a default, higher than minimum
        doubleProperty = Double(minInc=2.1, defaultValue=5.2)
        value = get_default_value(doubleProperty, force=True)
        assert value == 5.2
        assert value.dtype == np.float64
        # 3. Check options
        doubleProperty = Double(options=[5.1, 6.2, 7.3])
        value = get_default_value(doubleProperty, force=True)
        assert value == 5.1
        # 4. No default value provided, we force to default, but 0
        doubleProperty = Double(minInc=-2.1)
        value = get_default_value(doubleProperty, force=True)
        assert value == 0.0

        # ------------------------------

        vectorHashProperty = VectorHash(rows=Row, minSize=2)
        with self.assertRaises(TypeError):
            get_default_value(vectorHashProperty, force=True)


if __name__ == "__main__":
    main()
