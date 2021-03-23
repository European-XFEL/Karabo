

from unittest import TestCase, main

from karabo.native import (
    AccessMode, Bool, Configurable, Double, Float, get_default_value, Int32,
    sanitize_table_schema, String, VectorDouble, VectorHash)


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


class TestTable(Configurable):
    """A Configurable with a Table Element."""
    table = VectorHash(
        rows=Row,
        displayedName="Table",
        accessMode=AccessMode.READONLY)


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

        # In place modification
        sanitize_table_schema(table_row)
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


if __name__ == "__main__":
    main()
