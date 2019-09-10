import unittest

from karabo.bound import (
    Configurator, Hash, Schema,
    INT32_ELEMENT, OVERWRITE_ELEMENT, PythonDevice,
)

from .device_with_alarm import DeviceWithAlarm
from .device_with_table_parameter import DeviceWithTableElementParam


class Schema_Injection_TestCase(unittest.TestCase):

    def test_schemaInjection(self):
        device = Configurator(PythonDevice).create(
                        "PythonDevice", Hash())
        device.startFsm()

        # Test appendSchema appends
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(1)
            .reconfigurable()
            .commit()
        )

        device.appendSchema(schema)
        device.parameters.set("injectedInt32", 5)
        self.assertIn("injectedInt32", device.parameters.getPaths())
        self.assertEqual(device.parameters.get("injectedInt32"), 5)

        # Test that injecting a new attribute keeps the set value
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(2)
            .reconfigurable().minInc(1)
            .commit()
        )

        device.appendSchema(schema)
        self.assertIn("injectedInt32", device.parameters.getPaths())
        self.assertEqual(device.parameters.get("injectedInt32"), 5)
        self.assertEqual(device.fullSchema.getMinInc("injectedInt32"), 1)

        # Test setting attributes
        schema = Schema()
        schema.copy(device.fullSchema)
        min_value = schema.getMinInc("heartbeatInterval")
        min_value_new = min_value + 1
        (
            OVERWRITE_ELEMENT(schema).key("heartbeatInterval")
            .setNewMinInc(min_value_new)
            .commit(),
        )

        device.updateSchema(schema)
        self.assertEqual(device.fullSchema.getMinInc("heartbeatInterval"),
                         min_value_new)

        # Test that attributes of static parameters are preserved by
        # appendSchema
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(1)
            .reconfigurable()
            .commit()
        )

        device.appendSchema(schema)
        self.assertEqual(device.fullSchema.getMinInc("heartbeatInterval"),
                         min_value_new)

        # Test that attributes are reset by updateSchema
        device.updateSchema(schema)
        self.assertEqual(device.fullSchema.getMinInc("heartbeatInterval"),
                         min_value)

        # Test that doing updateSchema keeps previously set value
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(3)
            .reconfigurable().minInc(2).maxInc(10)
            .commit()
        )

        device.updateSchema(schema)
        self.assertIn("injectedInt32", device.parameters.getPaths())
        self.assertEqual(device.parameters.get("injectedInt32"), 5)
        self.assertEqual(device.fullSchema.getMinInc("injectedInt32"), 2)
        self.assertEqual(device.fullSchema.getMaxInc("injectedInt32"), 10)

        # Test that doing updateSchema with something else loses
        # injectedInt32.
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.updateSchema(schema)
        self.assertNotIn("injectedInt32", device.parameters.getPaths())
        self.assertIn("somethingElse", device.parameters.getPaths())

        # Test that updateSchema a parameter three times keeps the original
        # value This is to ensure that the schema parsing check is correct
        device.set("somethingElse", 42)
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .assignmentOptional().defaultValue(5)
            .reconfigurable()
            .commit()
        )
        device.updateSchema(schema)

        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .readOnly()
            .commit()
        )
        device.updateSchema(schema)

        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .assignmentOptional().defaultValue(5)
            .minInc(3)
            .reconfigurable()
            .commit()
        )
        device.updateSchema(schema)
        self.assertEqual(device.get("somethingElse"), 42)

        # Test that doing updateSchema with an empty schema reset the
        # device to its base schema
        schema = Schema()
        device.updateSchema(schema)

        self.assertNotIn("somethingElse", device.parameters.getPaths())
        self.assertEqual(device._injectedSchema.getPaths(),
                         Schema().getPaths())
        self.assertEqual(device.fullSchema.getPaths(),
                         device.staticSchema.getPaths())

        # Test that appendSchema parameter three times keeps the original
        # value This is to ensure that the schema parsing check is correct
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .assignmentOptional().defaultValue(5)
            .reconfigurable()
            .commit()
        )
        device.appendSchema(schema)
        device.set("somethingElse", 42)

        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .readOnly()
            .commit()
        )
        device.appendSchema(schema)

        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingElse")
            .assignmentOptional().defaultValue(5)
            .minInc(3)
            .reconfigurable()
            .commit()
        )
        device.appendSchema(schema)
        self.assertEqual(device.get("somethingElse"), 42)

        # Test that appending several times in a row, quickly, set all values
        for idx in range(10):
            schema = Schema()
            (
                INT32_ELEMENT(schema).key(f"property{idx}")
                .assignmentOptional().defaultValue(idx)
                .reconfigurable()
                .commit()
            )
            device.appendSchema(schema)

        for idx in range(10):
            key = f"property{idx}"
            self.assertIn(key, device.parameters.getPaths())
            self.assertIn(key, device.fullSchema.getPaths())
            self.assertEqual(idx, device.parameters.get(key))

    def test_schemaWithTableElementUpdate(self):
        """Tests that updateSchema preserves TABLE_ELEMENTs in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithTableElementParam", Hash())
        device.startFsm()

        self.assertIn("deviceTable",
                      device.getSchema("DeviceWithTableElementParam").getPaths())

        # Test that doing updateSchema with something new keeps
        # the table element parameter.
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.updateSchema(schema)

        self.assertIn("deviceTable",
                      device.getSchema("DeviceWithTableElementParam").getPaths())
        table = device.parameters.get("deviceTable")
        self.assertTrue(len(table) == 2)

    def test_schemaWithTableElementAppend(self):
        """Tests that appendSchema preserves TABLE_ELEMENTs in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithTableElementParam", Hash())
        device.startFsm()

        self.assertIn("deviceTable",
                      device.getSchema("DeviceWithTableElementParam").getPaths())

        # Test that doing updateSchema with something new keeps
        # the table element parameter.
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.appendSchema(schema)

        self.assertIn("deviceTable",
                      device.getSchema("DeviceWithTableElementParam").getPaths())
        table = device.parameters.get("deviceTable")
        self.assertTrue(len(table) == 2)

    def test_schemaWithAlarmHighUpdate(self):
        """Tests that updateSchema resets alarms in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithAlarm", Hash())
        device.startFsm()

        alarm_high = 2 * DeviceWithAlarm.ALARM_HIGH
        device.fullSchema.setAlarmHigh("valueWithAlarm", alarm_high)
        self.assertEqual(device.fullSchema.getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Test that doing updateSchema with something new resets
        # the parameter alarm
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.updateSchema(schema)
        self.assertEqual(device.fullSchema.getAlarmHigh("valueWithAlarm"),
                         DeviceWithAlarm.ALARM_HIGH)

    def test_schemaWithAlarmHighAppend(self):
        """Tests that updateSchema preserves alarms in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithAlarm", Hash())
        device.startFsm()

        alarm_high = 2 * DeviceWithAlarm.ALARM_HIGH
        device.fullSchema.setAlarmHigh("valueWithAlarm", alarm_high)
        self.assertEqual(device.fullSchema.getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Test that doing updateSchema with something new keeps
        # the parameter alarm
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.appendSchema(schema)
        self.assertEqual(device.fullSchema.getAlarmHigh("valueWithAlarm"),
                         alarm_high)
