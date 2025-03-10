#############################################################################
# Author: degon & costar
# Created on August 21, 2020, 11:42 AM
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
#############################################################################
import os
from contextlib import contextmanager

import pytest

from karabo.common.services import KARABO_CONFIG_MANAGER as MANAGER
from karabo.middlelayer import (
    Device, Double, Hash, HashList, KaraboError, Slot, String, call,
    connectDevice, getConfigurationFromName, instantiateFromName,
    listConfigurationFromName, listDevicesWithConfiguration,
    saveConfigurationFromName, slot)
from karabo.middlelayer.tests.eventloop import DeviceTest, async_tst

from ..configuration_manager import ConfigurationManager

DB_NAME = "test_karabo_db"

conf = {
    "classId": "ConfigurationManager",
    "_deviceId_": MANAGER,
    "dbName": DB_NAME
}

conf_alice = {
    "_deviceId_": "ALICE"
}

conf_bob = {
    "_deviceId_": "BOB"
}

conf_charlie = {
    "_deviceId_": "CHARLIE"
}


class TestDevice(Device):
    value = Double(
        defaultValue=5.0,
        warnHigh=100.0)


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

    @slot
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


@pytest.mark.skip("Refactoring of Config DB ongoing")
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
        cls.testDev = TestDevice(conf_alice)
        cls.anotherTestDev = TestDevice(conf_bob)
        cls.clientDev = TestDevice(conf_charlie)
        with cls.deviceManager(cls.testDev, cls.clientDev, cls.anotherTestDev,
                               lead=cls.dev):
            yield

    @async_tst
    async def test_configuration_save(self):
        """Test the manual saving of configurations"""
        config_name = "testConfig"

        attrs = [Hash("path", "value", "attribute", "warnHigh", "value", 50.0)]
        r = await call("ALICE", "slotUpdateSchemaAttributes", attrs)
        self.assertEqual(self.testDev.value.descriptor.warnHigh, 50.0)
        h = Hash("name", config_name, "deviceIds", ["ALICE"],
                 "priority", 3)
        r = await call(MANAGER, "slotSaveConfigurationFromName", h)
        self.assertEqual(r["success"], True)
        config_name = "testConfig1"
        h = Hash("name", config_name, "deviceIds", ["ALICE",
                                                    "BOB"],
                 "priority", 2)
        r = await call(MANAGER, "slotSaveConfigurationFromName", h)
        self.assertEqual(r["success"], True)

        invalid_config_name = "s#invalid"
        h = Hash("name", invalid_config_name, "deviceIds", ["ALICE",
                                                            "BOB"],
                 "priority", 2)
        with self.assertRaises(KaraboError):
            await call(MANAGER, "slotSaveConfigurationFromName", h)

        long_name = "s" * 40
        h = Hash("name", long_name, "deviceIds", ["ALICE",
                                                  "BOB"],
                 "priority", 2)
        with self.assertRaises(KaraboError):
            await call(MANAGER, "slotSaveConfigurationFromName", h)

    @async_tst
    async def test_configuration_save_check(self):
        """Cross check if we saved the config"""
        config_name = "testConfig1"
        h = Hash("name", config_name, "deviceIds", ["ALICE",
                                                    "BOB"])
        r = await call(MANAGER, "slotCheckConfigurationFromName", h)
        self.assertEqual(r["success"], True)
        self.assertEqual(r["taken"], True)

        # Wrong config name
        config_name = "NotSavedConfig"
        h = Hash("name", config_name, "deviceIds", ["ALICE",
                                                    "BOB"])
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

        h = Hash("priority", 3)
        r = await call(MANAGER, "slotListDevices", h)
        self.assertEqual(r["success"], True)
        devices = r["item"]
        self.assertNotIn("FOODEVICE", devices)
        self.assertIn("ALICE", devices)
        self.assertNotIn("BOB", devices)

        h = Hash("priority", 2)
        r = await call(MANAGER, "slotListDevices", h)
        self.assertEqual(r["success"], True)
        devices = r["item"]
        self.assertNotIn("FOODEVICE", devices)
        self.assertIn("ALICE", devices)
        self.assertIn("BOB", devices)

    @async_tst
    async def test_configuration_bulk_save_reject(self):
        """Test the reject of saving of too many configurations"""
        deviceIds = ["ALICE"] * 31
        config_name = "testConfig"
        h = Hash("name", config_name, "deviceIds", deviceIds,
                 "priority", 3)
        with self.assertRaises(KaraboError):
            await call(MANAGER, "slotSaveConfigurationFromName", h)

    @async_tst
    async def test_get_configuration(self):
        """Test the manual retrieving of configurations"""
        config_name = "testConfig"
        h = Hash("name", config_name, "deviceId", "ALICE", "schema",
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

    @async_tst
    async def test_list_configuration(self):
        h = Hash("name", "", "deviceId", "ALICE")
        r = await call(MANAGER, "slotListConfigurationFromName", h)
        items = r["items"]
        self.assertIsInstance(items, HashList)
        # We stored two configurations!
        self.assertEqual(len(items), 2)
        item = items[0]
        self.assertEqual(item["name"], "testConfig")
        item = items[1]
        self.assertEqual(item["name"], "testConfig1")

        h = Hash("name", "", "deviceId", "BOB")
        r = await call(MANAGER, "slotListConfigurationFromName", h)
        items = r["items"]
        self.assertIsInstance(items, HashList)
        # We stored one configuration!
        self.assertEqual(len(items), 1)

    @async_tst
    async def test_list_configuration_sets(self):
        devices = ["ALICE", "BOB"]
        h = Hash("deviceIds", devices)
        r = await call(MANAGER, "slotListConfigurationSets", h)
        items = r["items"]
        # Just one succesful save for both under same setId
        self.assertEqual(len(items), 1)
        config_set = items[0]
        self.assertEqual(config_set["name"], "testConfig1")
        self.assertEqual(config_set["diff_timepoint"], 0.0)
        self.assertEqual(config_set["description"], "")
        self.assertEqual(config_set["user"], ".")

    @async_tst
    async def test_zhe_get_last_configuration(self):
        """The typo is intended as order of tests is alphabetically done"""
        h = Hash("deviceId", "ALICE")
        r = await call(MANAGER,
                       "slotListConfigurationFromName", h)
        items = r["items"]
        # We stored two configurations!
        self.assertEqual(len(items), 2)

        # Get the last configuration!
        h = Hash("deviceId", "ALICE", "priority", 3, "schema", True)
        r = await call(MANAGER, "slotGetLastConfiguration", h)

        item = r["item"]
        name = item["name"]
        self.assertEqual(name, "testConfig")
        config = item["config"]
        value = config["value"]
        self.assertEqual(value, 5.0)

        # Get the last config without schema!
        h = Hash("deviceId", "ALICE", "priority", 3)
        r = await call(MANAGER, "slotGetLastConfiguration", h)
        item = r["item"]
        self.assertNotIn("schema", item)

    @async_tst
    async def test_get_client_configuration(self):
        """Test the functions that can be used from ikarabo"""
        # Device client functions
        await saveConfigurationFromName(
            "CHARLIE", name="testConfigClient",
            description="No desc", priority=3)
        await saveConfigurationFromName(
            ["CHARLIE"], name="testConfigClientList",
            description="Client list", priority=1)

        dev = await connectDevice("CHARLIE")
        await saveConfigurationFromName(
            ["CHARLIE", dev], name="testConfigClientMixedList",
            description="Mixed", priority=2)
        li = await listConfigurationFromName("CHARLIE")
        self.assertEqual(len(li), 3)
        self.assertIsInstance(li, HashList)
        clist = ["testConfigClient", "testConfigClientList",
                 "testConfigClientMixedList"]
        self.assertIn(li[0]["name"], clist)
        self.assertIn(li[1]["name"], clist)
        config = await getConfigurationFromName("CHARLIE",
                                                "testConfigClientList")
        value = config["value"]
        self.assertEqual(value, 5.0)

        # item = await call(MANAGER, "slotGetLastConfiguration", h)
        item = {}
        # item = await getLastConfiguration("CHARLIE", priority=3)
        self.assertEqual(item["name"], "testConfigClient")
        config = item["config"]
        value = config["value"]
        self.assertEqual(value, 5.0)
        self.assertEqual(item["priority"], 3)
        self.assertEqual(item["user"], ".")
        self.assertEqual(item["description"], "No desc")

        deviceIds = await listDevicesWithConfiguration()
        self.assertIn("CHARLIE", deviceIds)
        self.assertIn("ALICE", deviceIds)
        self.assertNotIn("BOB", deviceIds)

        deviceIds = await listDevicesWithConfiguration(priority=2)
        self.assertIn("CHARLIE", deviceIds)
        self.assertIn("ALICE", deviceIds)
        self.assertIn("BOB", deviceIds)

        # Instantiate a device with a device client function
        serverMock = MockServer({"_deviceId_": "MDL_SERVER"})
        await serverMock.startInstance()
        try:
            self.assertEqual(serverMock.lastClassId, "")
            self.assertEqual(serverMock.lastDeviceId, "")
            self.assertEqual(serverMock.lastConfigDouble, DEFAULT_LAST_DOUBLE)
            await instantiateFromName("ALICE", name="testConfig",
                                      serverId="MDL_SERVER")
            self.assertEqual(serverMock.lastClassId, "TestDevice")
            self.assertEqual(serverMock.lastDeviceId, "ALICE")
            self.assertEqual(serverMock.lastConfigDouble, 5.0)
        finally:
            await serverMock.slotKillDevice()

    @async_tst
    async def test_instantiate_device(self):
        serverMock = MockServer({"_deviceId_": "MDL_SERVER"})
        await serverMock.startInstance()
        try:
            h = Hash()
            h["deviceId"] = "ALICE"
            h["name"] = "testConfig"
            h["serverId"] = "MDL_SERVER"
            await call(MANAGER, "slotInstantiateDevice", h)
            self.assertEqual(serverMock.lastClassId, "TestDevice")
            self.assertEqual(serverMock.lastDeviceId, "ALICE")
            self.assertEqual(serverMock.lastConfigDouble, 5.0)

            # reset settings
            await serverMock.reset()
            self.assertEqual(serverMock.lastClassId, "")
            self.assertEqual(serverMock.lastDeviceId, "")
            self.assertEqual(serverMock.lastConfigDouble, DEFAULT_LAST_DOUBLE)

            h = Hash()
            h["deviceId"] = "ALICE"
            h["name"] = "testConfig"
            h["classId"] = "NoClass"
            h["serverId"] = "MDL_SERVER"
            # ClassId missmatch ...
            with self.assertRaises(KaraboError):
                await call(MANAGER, "slotInstantiateDevice", h)

            h = Hash()
            h["deviceId"] = "ALICE"
            h["serverId"] = "MDL_SERVER"
            await call(MANAGER, "slotInstantiateDevice", h)
            self.assertEqual(serverMock.lastClassId, "TestDevice")
            self.assertEqual(serverMock.lastDeviceId, "ALICE")
            self.assertEqual(serverMock.lastConfigDouble, 5.0)
        finally:
            await serverMock.slotKillDevice()
