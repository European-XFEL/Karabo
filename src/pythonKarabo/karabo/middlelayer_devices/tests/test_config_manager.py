#############################################################################
# Author: degon
# Created on August 21, 2020, 11:42 AM
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
import os

from karabo.middlelayer_api.tests.eventloop import async_tst, DeviceTest
from karabo.middlelayer import (
    call, connectDevice, DaqPolicy, Device, Double, getConfigurationFromName,
    getLastConfiguration, Hash, HashList,
    listConfigurationFromName, saveConfigurationFromName, sleep)
from karabo.middlelayer_devices.configuration_manager import (
    ConfigurationManager)


DB_NAME = "test_karabo_db"

TEST_MANAGER = "KaraboConfigurationManager"

conf = {
    "classId": "ConfigurationManager",
    "_deviceId_": TEST_MANAGER,
    "dbName": DB_NAME
}


conf_test_device = {
    "_deviceId_": "TEST_DEVICE"
}

conf_client_device = {
    "_deviceId_": "TEST_CLIENT_DEVICE"
}


class TestDevice(Device):
    value = Double(
        defaultValue=5.0,
        daqPolicy=DaqPolicy.SAVE)


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
        cls.clientDev = TestDevice(conf_client_device)
        with cls.deviceManager(cls.testDev, cls.clientDev, lead=cls.dev):
            yield

    @async_tst
    async def test_configuration_save(self):
        # Manual saving
        config_name = "testConfig"
        h = Hash("name", config_name, "deviceIds", ["TEST_DEVICE"],
                 "priority", 3)
        r = await call(TEST_MANAGER, "slotSaveConfigurationFromName", h)
        input_hash = r["input"]
        self.assertEqual(input_hash, h)
        sleep(0.1)
        config_name = "testConfig1"
        h = Hash("name", config_name, "deviceIds", ["TEST_DEVICE"],
                 "priority", 2)
        r = await call(TEST_MANAGER, "slotSaveConfigurationFromName", h)
        input_hash = r["input"]
        self.assertEqual(input_hash, h)

    @async_tst
    async def test_get_configuration(self):
        config_name = "testConfig"
        h = Hash("name", config_name, "deviceId", "TEST_DEVICE")
        r = await call(TEST_MANAGER, "slotGetConfigurationFromName", h)
        input_hash = r["input"]
        self.assertEqual(input_hash, h)
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
        r = await call(TEST_MANAGER, "slotListConfigurationFromName", h)
        input_hash = r["input"]
        self.assertEqual(input_hash, h)
        items = r["items"]
        self.assertIsInstance(items, HashList)
        # We stored two configurations!
        self.assertEqual(len(items), 2)
        item = items[0]
        self.assertEqual(item["name"], "testConfig")
        item = items[1]
        self.assertEqual(item["name"], "testConfig1")

    @async_tst
    async def test_z_get_last_configuration(self):
        h = Hash("deviceId", "TEST_DEVICE")
        r = await call(TEST_MANAGER, "slotListConfigurationFromName", h)
        input_hash = r["input"]
        self.assertEqual(input_hash, h)
        items = r["items"]
        # We stored two configurations!
        self.assertEqual(len(items), 2)

        # Get the last configuration!
        h = Hash("deviceId", "TEST_DEVICE", "priority", 3)
        r = await call(TEST_MANAGER, "slotGetLastConfiguration",  h)
        input_hash = r["input"]
        self.assertEqual(input_hash, h)

        item = r["item"]
        name = item["name"]
        self.assertEqual(name, "testConfig")
        config = item["config"]
        value = config["value"]
        self.assertEqual(value, 5.0)
        schema = item["schema"]
        policy = schema.hash["value", "daqPolicy"]
        self.assertEqual(policy, DaqPolicy.SAVE)

    @async_tst
    async def test_device_client_configuration(self):
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