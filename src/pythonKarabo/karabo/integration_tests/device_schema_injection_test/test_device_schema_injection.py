import unittest

from karabo.bound import (
    Configurator, Hash, Schema,
    DOUBLE_ELEMENT, INT32_ELEMENT, PythonDevice, VectorHash
)

from .device_with_alarm import DeviceWithAlarm
from .device_with_table_parameter import DeviceWithTableElementParam


class Schema_Injection_TestCase(unittest.TestCase):
    deviceCfg = Hash('Logger.priority', "FATAL")

    def test_schemaInjection(self):
        device = Configurator(PythonDevice).create(
                        "PythonDevice", self.deviceCfg)
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
        self.assertEqual(device.get("injectedInt32"), 1)  # the default
        device.set("injectedInt32", 5)
        self.assertEqual(device.get("injectedInt32"), 5)

        # Test validation exception to raise while attempting to set property
        # not defined in the schema, for example "unknownProperty" ...
        with self.assertRaises(RuntimeError):
            # attempt to set "unknownProperty"
            device.set("unknownProperty", 42)
        currentConfig = device.getCurrentConfiguration()
        self.assertNotIn("unknownProperty", currentConfig.getPaths())

        # Test that injecting a new attribute keeps the set value
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(2)
            .reconfigurable().minInc(1)
            .commit()
        )

        device.appendSchema(schema)
        self.assertEqual(device.get("injectedInt32"), 5)  # not the new default
        self.assertEqual(device.getFullSchema().getMinInc("injectedInt32"), 1)

        # Test that doing updateSchema keeps previously set value
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(3)
            .reconfigurable().minInc(2).maxInc(10)
            .commit()
        )

        device.updateSchema(schema)
        self.assertEqual(device.get("injectedInt32"), 5)
        self.assertEqual(device.getFullSchema().getMinInc("injectedInt32"), 2)
        self.assertEqual(device.getFullSchema().getMaxInc("injectedInt32"), 10)

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
        currentParams = device.getCurrentConfiguration()
        self.assertNotIn("injectedInt32", currentParams.getPaths())
        self.assertIn("somethingElse", currentParams.getPaths())

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

        self.assertNotIn("somethingElse",
                         device.getCurrentConfiguration().getPaths())
        # We are back at static Schema without "somethingElse"
        self.assertNotIn("somethingElse",
                         device.getFullSchema().getPaths())
        self.assertEqual(device.getFullSchema().getPaths(),
                         PythonDevice.getSchema(device.classid).getPaths())

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
            self.assertIn(key, device.getCurrentConfiguration().getPaths())
            self.assertIn(key, device.getFullSchema().getPaths())
            self.assertEqual(idx, device.get(key))

    def test_schemaWithTableElementUpdate(self):
        """Tests that updateSchema preserves TABLE_ELEMENTs in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithTableElementParam", self.deviceCfg)
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
        table = device.get("deviceTable")
        self.assertTrue(len(table) == 2)

    def test_schemaWithTableElementAppend(self):
        """Tests that appendSchema preserves TABLE_ELEMENTs in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithTableElementParam", self.deviceCfg)
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
        table = device.get("deviceTable")
        self.assertTrue(len(table) == 2)

    def test_schemaWithAttributeUpdate(self):
        """Tests that updateSchema resets attributes in the static schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithAlarm", self.deviceCfg)
        device.startFsm()

        # Update the alarmHigh
        schema = Schema()
        alarm_high = 2 * DeviceWithAlarm.ALARM_HIGH
        (
            DOUBLE_ELEMENT(schema).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(alarm_high).needsAcknowledging(False)
            .commit(),
        )
        device.updateSchema(schema)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Test that doing updateSchema with something new resets
        # the alarmHigh
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.updateSchema(schema)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         DeviceWithAlarm.ALARM_HIGH)

        # Update the alarmHigh by using slotUpdateSchemaAttributes,
        # as the guiServer would do when instantiating a device
        alarm_high *= 2
        vh = VectorHash()
        g = Hash("path", "valueWithAlarm", "attribute", "alarmHigh",
                 "value", alarm_high)
        vh.append(g)
        device.slotUpdateSchemaAttributes(vh)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Test that doing updateSchema with something new resets
        # the alarmHigh
        device.updateSchema(schema)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         DeviceWithAlarm.ALARM_HIGH)

    def test_schemaWithAttributeAppend(self):
        """Tests that appendSchema preserves attributes in the static
        schema."""
        device = Configurator(PythonDevice).create(
                        "DeviceWithAlarm", self.deviceCfg)
        device.startFsm()

        # Update the alarmHigh
        schema = Schema()
        alarm_high = 2 * DeviceWithAlarm.ALARM_HIGH
        (
            DOUBLE_ELEMENT(schema).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(alarm_high).needsAcknowledging(False)
            .commit(),
        )
        device.updateSchema(schema)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Test that doing appendSchema with something new keeps
        # the alarmHigh
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit()
        )

        device.appendSchema(schema)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Update the alarmHigh by using slotUpdateSchemaAttributes,
        # as the guiServer would do when instantiating a device
        alarm_high *= 2
        vh = VectorHash()
        g = Hash("path", "valueWithAlarm", "attribute", "alarmHigh",
                 "value", alarm_high)
        vh.append(g)
        device.slotUpdateSchemaAttributes(vh)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         alarm_high)

        # Test that doing appendSchema with something new keeps
        # the alarmHigh
        device.appendSchema(schema)
        self.assertEqual(device.getFullSchema().getAlarmHigh("valueWithAlarm"),
                         alarm_high)

    def test_ensureGetCopies(self):
        """
        Hijack schema test to verify that PythonDevice.get(..) does not return
        anything that can be used to change PythonDevice._parameters
        (since that would need lock protection).
        Similar for getFullSchema()
        """
        device = Configurator(PythonDevice).create(
                        "DeviceWithAlarm", self.deviceCfg)
        device.startFsm()

        # VECTOR_ELEMENT
        vec = device.get("vector")
        self.assertEqual(vec, [0])  # default just contains 0
        vec[0] = 1
        self.assertEqual(vec, [1])
        self.assertEqual(device.get("vector")[0], 0)

        # Get a full NODE_ELEMENT
        node = device.get("node")
        self.assertEqual(node["number"], 0)
        # Set something in node - should not affect device value
        node["number"] = 100
        self.assertEqual(node["number"], 100)
        self.assertEqual(device.get("node.number"), 0)

        # TABLE_ELEMENT
        table = device.get("table")
        self.assertEqual(len(table), 2)  # default
        self.assertEqual(table[0]["int32"], 1)  # default
        self.assertEqual(table[1]["int32"], 2)  # default
        # test setting value inside row
        table[1]["int32"] = 3
        self.assertEqual(table[1]["int32"], 3)
        self.assertEqual(device.get("table")[1]["int32"], 2)
        # test replacing a row
        table[0] = Hash("int32", -1)
        self.assertEqual(table[0]["int32"], -1)
        self.assertEqual(device.get("table")[0]["int32"], 1)
        # test appending a row
        table.append(Hash("int32", 99))
        self.assertEqual(len(table), 3)
        self.assertEqual(len(device.get("table")), 2)

        # Go on testing that getFullSchema() returns a copy
        schema = device.getFullSchema()
        (
            INT32_ELEMENT(schema).key("aNewKeyNotExisting")
            .assignmentOptional().defaultValue(1)
            .commit(),
        )
        self.assertTrue(schema.has("aNewKeyNotExisting"))
        # So schema was changed, but device's schema not:
        self.assertFalse(device.getFullSchema().has("aNewKeyNotExisting"))
