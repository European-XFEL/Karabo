#############################################################################
# Author: degon
# Created on August 21, 2020, 11:42 AM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
import os

from karabo.common.services import KARABO_CONFIG_MANAGER as MANAGER
from karabo.middlelayer_api.tests.eventloop import async_tst, DeviceTest
from karabo.middlelayer import (
    call, coslot, connectDevice, DaqPolicy, Device, Double,
    getConfigurationFromName, getLastConfiguration, Hash, HashList,
    instantiateFromName, KaraboError, listConfigurationFromName,
    saveConfigurationFromName, sleep, Slot, slot, String)
from karabo.middlelayer_devices.configuration_manager import (
    ConfigurationManager)

DB_NAME = "test_karabo_db"

conf = {
    "classId": "ConfigurationManager",
    "_deviceId_": MANAGER,
    "dbName": DB_NAME
}

conf_test_device = {
    "_deviceId_": "TEST_DEVICE"
}

conf_another_test_device = {
    "_deviceId_": "ANOTHER_TEST_DEVICE"
}

conf_client_device = {
    "_deviceId_": "TEST_CLIENT_DEVICE"
}


class TestDevice(Device):
    value = Double(
        defaultValue=5.0,
        daqPolicy=DaqPolicy.SAVE,
        maxInc=100.0)


DEFAULT_LAST_DOUBLE = -1.0


class MockServer(Device):
    lastClassId = String(
        defaultValue=""
    )

    lastDeviceId = String(
        defaultValue=""
    )

    lastConfigDouble = Double(
        defaultValue=DEFAULT_LAST_DOUBLE
    )

    @Slot()
    async def reset(self):
        self.lastClassId = ""
        self.lastConfigDouble = DEFAULT_LAST_DOUBLE
        self.lastDeviceId = ""

    @coslot
    async def slotStartDevice(self, info):
        """The info hash should contain

        - configuration
        - deviceId
        - classId
        """
        deviceId = info["deviceId"]
        config = info["configuration"]
        classId = info["classId"]

        self.lastDeviceId = deviceId
        self.lastClassId = classId
        self.lastConfigDouble = config.get('value', DEFAULT_LAST_DOUBLE)

        return True, self.deviceId

    @slot
    def slotGetClassSchema(self, classId):
        """Fake the retrieval of a class schema retrieval"""
        schema = TestDevice.getClassSchema()
        return schema, "TestDevice", self.deviceId


class TestConfigurationManager(DeviceTest):

    @classmethod
    def tearDownClass(cls):
        db = cls.dev.db.path
        os.remove(db)
        super(cls, cls).tearDownClass()

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = ConfigurationManager(conf)
        cls.testDev = TestDevice(conf_test_device)
        cls.anotherTestDev = TestDevice(conf_another_test_device)
        cls.clientDev = TestDevice(conf_client_device)
        with cls.deviceManager(cls.testDev, cls.clientDev, cls.anotherTestDev,
                               lead=cls.dev):
            yield

    @async_tst
    async def test_configuration_save(self):
        """Test the manual saving of configurations"""
        config_name = "testConfig"

        attrs = [Hash("path", "value", "attribute", "maxInc", "value", 50.0)]
        r = await call("TEST_DEVICE", "slotUpdateSchemaAttributes", attrs)
        self.assertEqual(self.testDev.value.descriptor.maxInc, 50.0)
        h = Hash("name", config_name, "deviceIds", ["TEST_DEVICE"],
                 "priority", 3)
        r = await call(MANAGER, "slotSaveConfigurationFromName", h)
        self.assertEqual(r["success"], True)
        config_name = "testConfig1"
        h = Hash("name", config_name, "deviceIds", ["TEST_DEVICE",
                                                    "ANOTHER_TEST_DEVICE"],
                 "priority", 2)
        r = await call(MANAGER, "slotSaveConfigurationFromName", h)
        self.assertEqual(r["success"], True)

        invalid_config_name = "s#invalid"
        h = Hash("name", invalid_config_name, "deviceIds", ["TEST_DEVICE",
                                                            "ANOTHER_TEST_DEVICE"],
                 "priority", 2)
        with self.assertRaises(KaraboError):
            await call(MANAGER, "slotSaveConfigurationFromName", h)

        long_name = "s" * 40
        h = Hash("name", long_name, "deviceIds", ["TEST_DEVICE",
                                                  "ANOTHER_TEST_DEVICE"],
                 "priority", 2)
        with self.assertRaises(KaraboError):
            await call(MANAGER, "slotSaveConfigurationFromName", h)

    @async_tst
    async def test_configuration_save_check(self):
        """Cross check if we saved the config"""
        config_name = "testConfig1"
        h = Hash("name", config_name, "deviceIds", ["TEST_DEVICE",
                                                    "ANOTHER_TEST_DEVICE"])
        r = await call(MANAGER, "slotCheckConfigurationFromName", h)
        self.assertEqual(r["success"], True)
        self.assertEqual(r["taken"], True)

        # Wrong config name
        config_name = "NotSavedConfig"
        h = Hash("name", config_name, "deviceIds", ["TEST_DEVICE",
                                                    "ANOTHER_TEST_DEVICE"])
        r = await call(MANAGER, "slotCheckConfigurationFromName", h)
        self.assertEqual(r["success"], True)
        self.assertEqual(r["taken"], False)

        # Wrong device names
        config_name = "testConfig1"
        h = Hash("name", config_name, "deviceIds", ["FOODEVICE",
                                                    "BARDEVICE"])
        r = await call(MANAGER, "slotCheckConfigurationFromName", h)
        self.assertEqual(r["success"], True)
        self.assertEqual(r["taken"], False)

    @async_tst
    async def test_configuration_bulk_save_reject(self):
        """Test the reject of saving of too many configurations"""
        deviceIds = ["TEST_DEVICE"] * 31
        config_name = "testConfig"
        h = Hash("name", config_name, "deviceIds", deviceIds,
                 "priority", 3)
        with self.assertRaises(KaraboError):
            await call(MANAGER, "slotSaveConfigurationFromName", h)

    @async_tst
    async def test_get_configuration(self):
        """Test the manual retrieving of configurations"""
        config_name = "testConfig"
        h = Hash("name", config_name, "deviceId", "TEST_DEVICE", "schema",
                 True)
        r = await call(MANAGER, "slotGetConfigurationFromName", h)
        item = r["item"]
        priority = item["priority"]
        self.assertEqual(priority, 3)
        name = item["name"]
        self.assertEqual(name, config_name)
        config = item["config"]
        value = config["value"]
        self.assertEqual(value, 5.0)
        schema = item["schema"]
        policy = schema.hash["value", "daqPolicy"]
        self.assertEqual(policy, DaqPolicy.SAVE)

    @async_tst
    async def test_list_configuration(self):
        h = Hash("name", "", "deviceId", "TEST_DEVICE")
        r = await call(MANAGER, "slotListConfigurationFromName", h)
        items = r["items"]
        self.assertIsInstance(items, HashList)
        # We stored two configurations!
        self.assertEqual(len(items), 2)
        item = items[0]
        self.assertEqual(item["name"], "testConfig")
        item = items[1]
        self.assertEqual(item["name"], "testConfig1")

        h = Hash("name", "", "deviceId", "ANOTHER_TEST_DEVICE")
        r = await call(MANAGER, "slotListConfigurationFromName", h)
        items = r["items"]
        self.assertIsInstance(items, HashList)
        # We stored one configuration!
        self.assertEqual(len(items), 1)

    @async_tst
    async def test_list_configuration_sets(self):
        h = Hash("deviceIds", ["TEST_DEVICE", "ANOTHER_TEST_DEVICE"])
        r = await call(MANAGER, "slotListConfigurationSets", h)
        items = r["items"]
        # The other one for TEST_DEVICE is off in time.
        self.assertEqual(len(items), 1)
        config_set = items[0]
        self.assertEqual(config_set["name"], "testConfig1")
        self.assertEqual(config_set["diff_timepoint"], 0.0)
        self.assertEqual(config_set["description"], "")
        self.assertEqual(config_set["user"], ".")

    @async_tst
    async def test_zhe_get_last_configuration(self):
        """The typo is intended as order of tests is alphabetically done"""
        h = Hash("deviceId", "TEST_DEVICE")
        r = await call(MANAGER,
                       "slotListConfigurationFromName", h)
        items = r["items"]
        # We stored two configurations!
        self.assertEqual(len(items), 2)

        # Get the last configuration!
        h = Hash("deviceId", "TEST_DEVICE", "priority", 3, "schema", True)
        r = await call(MANAGER, "slotGetLastConfiguration", h)

        item = r["item"]
        name = item["name"]
        self.assertEqual(name, "testConfig")
        config = item["config"]
        value = config["value"]
        self.assertEqual(value, 5.0)
        schema = item["schema"]
        policy = schema.hash["value", "daqPolicy"]
        self.assertEqual(policy, DaqPolicy.SAVE)

        # Get the last config without schema!
        h = Hash("deviceId", "TEST_DEVICE", "priority", 3)
        r = await call(MANAGER, "slotGetLastConfiguration", h)
        item = r["item"]
        self.assertNotIn("schema", item)

    @async_tst
    async def test_device_client_configuration(self):
        """Test the functions that can be used from ikarabo"""
        # Device client functions
        await saveConfigurationFromName(
            "TEST_CLIENT_DEVICE", name="testConfigClient",
            description="No desc", priority=3)
        await saveConfigurationFromName(
            ["TEST_CLIENT_DEVICE"], name="testConfigClientList",
            description="Client list", priority=1)

        dev = await connectDevice("TEST_CLIENT_DEVICE")
        await saveConfigurationFromName(
            ["TEST_CLIENT_DEVICE", dev], name="testConfigClientMixedList",
            description="Mixed", priority=2)
        l = await listConfigurationFromName("TEST_CLIENT_DEVICE")
        self.assertEqual(len(l), 3)
        self.assertIsInstance(l, HashList)
        clist = ["testConfigClient", "testConfigClientList",
                 "testConfigClientMixedList"]
        self.assertIn(l[0]["name"], clist)
        self.assertIn(l[1]["name"], clist)
        config = await getConfigurationFromName("TEST_CLIENT_DEVICE",
                                                "testConfigClientList")
        value = config["value"]
        self.assertEqual(value, 5.0)

        item = await getLastConfiguration("TEST_CLIENT_DEVICE", priority=3)
        self.assertEqual(item["name"], "testConfigClient")
        config = item["config"]
        value = config["value"]
        self.assertEqual(value, 5.0)
        self.assertEqual(item["priority"], 3)
        self.assertEqual(item["user"], ".")
        self.assertEqual(item["description"], "No desc")

        # Instantiate a device with a device client function
        serverMock = MockServer({"_deviceId_": "TEST_SERVER"})
        await serverMock.startInstance()
        try:
            self.assertEqual(serverMock.lastClassId, "")
            self.assertEqual(serverMock.lastDeviceId, "")
            self.assertEqual(serverMock.lastConfigDouble, DEFAULT_LAST_DOUBLE)
            await instantiateFromName("TEST_DEVICE", name="testConfig",
                                      serverId="TEST_SERVER")
            self.assertEqual(serverMock.lastClassId, "TestDevice")
            self.assertEqual(serverMock.lastDeviceId, "TEST_DEVICE")
            self.assertEqual(serverMock.lastConfigDouble, 5.0)
        finally:
            await serverMock.slotKillDevice()

    @async_tst
    async def test_instantiate_device(self):
        serverMock = MockServer({"_deviceId_": "TEST_SERVER"})
        await serverMock.startInstance()
        try:
            h = Hash()
            h["deviceId"] = "TEST_DEVICE"
            h["name"] = "testConfig"
            h["serverId"] = "TEST_SERVER"
            attrs = [Hash("path", "value", "attribute",
                          "maxInc", "value", 200.0)]
            await call("TEST_DEVICE", "slotUpdateSchemaAttributes", attrs)
            self.assertEqual(self.testDev.value.descriptor.maxInc, 200.0)
            # Note: This is sneaky and a mind game. We have a fake server
            # that we call to instantiate a device which is already online
            # Hence, we can check if this was called to instantiate, but also
            # we can check if we runtime attributes have been applied to the
            # online device.

            await call(MANAGER, "slotInstantiateDevice", h)
            self.assertEqual(serverMock.lastClassId, "TestDevice")
            self.assertEqual(serverMock.lastDeviceId, "TEST_DEVICE")
            self.assertEqual(serverMock.lastConfigDouble, 5.0)
            self.assertEqual(self.testDev.value.descriptor.maxInc, 50.0)

            # attribute settings
            await serverMock.reset()
            self.assertEqual(serverMock.lastClassId, "")
            self.assertEqual(serverMock.lastDeviceId, "")
            self.assertEqual(serverMock.lastConfigDouble, DEFAULT_LAST_DOUBLE)

            h = Hash()
            h["deviceId"] = "TEST_DEVICE"
            h["name"] = "testConfig"
            h["classId"] = "NoClass"
            h["serverId"] = "TEST_SERVER"
            # ClassId missmatch ...
            with self.assertRaises(KaraboError):
                await call(MANAGER, "slotInstantiateDevice", h)

            h = Hash()
            h["deviceId"] = "TEST_DEVICE"
            h["serverId"] = "TEST_SERVER"
            await call(MANAGER, "slotInstantiateDevice", h)
            self.assertEqual(serverMock.lastClassId, "TestDevice")
            self.assertEqual(serverMock.lastDeviceId, "TEST_DEVICE")
            self.assertEqual(serverMock.lastConfigDouble, 5.0)
        finally:
            await serverMock.slotKillDevice()
