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

import numpy as np
import pytest
import pytest_asyncio

from karabo.common.services import KARABO_CONFIG_MANAGER as MANAGER_ID
from karabo.middlelayer import (
    Device, Double, Hash, HashByte, HashList, KaraboError, Schema, Slot,
    String, call, connectDevice, execute, getConfigurationFromName,
    getProperties, instantiateFromName, listConfigurationFromName,
    listDevicesWithConfiguration, saveConfigurationFromName, setWait, shutdown,
    slot)
from karabo.middlelayer.testing import AsyncDeviceContext

from ..configuration_manager import ConfigurationManager, hashToHash

DB_NAME = "test_karabo_db"

manager_conf = {
    "_deviceId_": MANAGER_ID,
    "dbName": DB_NAME,
    "isConfigMode": True,
}

conf_alice = {"_deviceId_": "ALICE"}
conf_bob = {"_deviceId_": "BOB"}
conf_charlie = {"_deviceId_": "CHARLIE"}


class MockingDevice(Device):
    value = Double(
        defaultValue=5.0)


UNDEFINED_LAST_DOUBLE = -1.0


class MockServer(Device):
    lastClassId = String(
        defaultValue="")

    lastDeviceId = String(
        defaultValue="")

    lastConfigDouble = Double(
        defaultValue=UNDEFINED_LAST_DOUBLE)

    def _initInfo(self):
        info = super()._initInfo()
        info["type"] = "server"
        return info

    @Slot()
    async def reset(self):
        self.lastClassId = ""
        self.lastConfigDouble = UNDEFINED_LAST_DOUBLE
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
        self.lastConfigDouble = config.get(
            'value', UNDEFINED_LAST_DOUBLE)

        return True, self.deviceId

    @slot
    def slotGetClassSchema(self, classId):
        """Fake the retrieval of a class schema retrieval"""
        schema = MockingDevice.getClassSchema()
        return schema, "MockingDevice", self.deviceId


@pytest_asyncio.fixture(loop_scope="module", scope="module")
async def deviceTest():
    dev = ConfigurationManager(manager_conf)
    dev._get_server_attributes = lambda deviceId: (
        "MDL_SERVER_MOCK", "MockingDevice")
    testDev = MockingDevice(conf_alice)
    anotherTestDev = MockingDevice(conf_bob)
    clientDev = MockingDevice(conf_charlie)
    serverMock = MockServer({"_deviceId_": "MDL_SERVER_MOCK"})
    async with AsyncDeviceContext(
        dev=dev, testDev=testDev, client=clientDev,
            another=anotherTestDev, serverMock=serverMock) as ctx:
        yield ctx

    db = dev.db
    await db.delete()


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_configuration_save(deviceTest):
    """Test the manual saving of configurations"""
    config_name = "testConfig"
    h = Hash("name", config_name, "deviceIds", ["ALICE"])
    r = await call(MANAGER_ID, "slotSaveConfigurationFromName", h)
    assert r["success"]
    await setWait("ALICE", value=10)
    config_name = "testConfig1"
    h = Hash("name", config_name, "deviceIds", ["ALICE",
                                                "BOB"])
    r = await call(MANAGER_ID, "slotSaveConfigurationFromName", h)
    assert r["success"]

    invalid_config_name = "s#invalid"
    h = Hash("name", invalid_config_name, "deviceIds", ["ALICE", "BOB"])
    with pytest.raises(KaraboError):
        await call(MANAGER_ID, "slotSaveConfigurationFromName", h)

    long_name = "s" * 40
    h = Hash("name", long_name, "deviceIds", ["ALICE", "BOB"])
    with pytest.raises(KaraboError):
        await call(MANAGER_ID, "slotSaveConfigurationFromName", h)


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_configuration_save_check(deviceTest):
    """Cross check if we saved the config"""
    h = Hash()
    r = await call(MANAGER_ID, "slotListDevices", h)
    assert r["success"]
    devices = r["item"]
    assert "FOODEVICE" not in devices
    assert "ALICE" in devices
    assert "BOB" in devices


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_configuration_bulk_save_reject(deviceTest):
    """Test the reject of saving of too many configurations"""
    deviceIds = ["ALICE"] * 31
    config_name = "testConfig"
    h = Hash("name", config_name, "deviceIds", deviceIds)
    with pytest.raises(KaraboError):
        await call(MANAGER_ID, "slotSaveConfigurationFromName", h)


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_get_configuration(deviceTest):
    """Test the manual retrieving of configurations"""
    config_name = "testConfig"
    h = Hash("name", config_name, "deviceId", "ALICE")
    r = await call(MANAGER_ID, "slotGetConfigurationFromName", h)
    item = r["item"]
    name = item["name"]
    assert name == config_name
    config = item["config"]
    assert "value" not in config

    # has been stored in previous test
    config_name = "testConfig1"
    h = Hash("name", config_name, "deviceId", "ALICE")
    r = await call(MANAGER_ID, "slotGetConfigurationFromName", h)
    item = r["item"]
    name = item["name"]
    assert name == config_name
    config = item["config"]
    assert "value" in config
    assert config["value"] == 10.0


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_list_delete_configuration(deviceTest):
    h = Hash("deviceId", "ALICE")
    r = await call(MANAGER_ID, "slotListConfigurationFromName", h)
    items = r["items"]
    assert isinstance(items, HashList)
    # We stored two configurations!
    assert len(items) == 2
    item = items[0]
    assert item["name"] == "testConfig"
    item = items[1]
    assert item["name"] == "testConfig1"

    h = Hash("deviceId", "ALICE", "name", "testConfig1")
    r = await call(MANAGER_ID, "slotDeleteConfiguration", h)
    assert r["success"]

    h = Hash("deviceId", "ALICE")
    r = await call(MANAGER_ID, "slotListConfigurationFromName", h)
    items = r["items"]
    assert isinstance(items, HashList)
    assert len(items) == 1

    h = Hash("name", "", "deviceId", "BOB")
    r = await call(MANAGER_ID, "slotListConfigurationFromName", h)
    items = r["items"]
    assert isinstance(items, HashList)
    # We stored one configuration!
    assert len(items) == 1


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_get_client_configuration(deviceTest):
    """Test the functions that can be used from ikarabo"""
    # Device client functions
    await setWait("CHARLIE", value=25.0)
    await saveConfigurationFromName(
        "CHARLIE", name="testConfigClient")
    await saveConfigurationFromName(
        ["CHARLIE"], name="testConfigClientList")

    dev = await connectDevice("CHARLIE")
    await saveConfigurationFromName(
        ["CHARLIE", dev], name="testConfigClientMixedList")
    config_list = await listConfigurationFromName("CHARLIE")
    assert len(config_list) == 3
    assert isinstance(config_list, HashList)
    clist = ["testConfigClient", "testConfigClientList",
             "testConfigClientMixedList"]

    for i in range(len(clist)):
        assert config_list[i]["name"] == clist[i]

    item = await getConfigurationFromName(
        "CHARLIE", "testConfigClientList")
    serverId = item["serverId"]
    classId = item["classId"]
    assert serverId == "MDL_SERVER_MOCK"
    assert classId == "MockingDevice"

    config = item["config"]
    value = config["value"]
    assert value == 25.0

    devices = await listDevicesWithConfiguration()
    assert "CHARLIE" in devices

    properties = await getProperties(
        "MDL_SERVER_MOCK",
        ["lastClassId", "lastDeviceId", "lastConfigDouble"])
    assert properties["lastClassId"] == ""
    assert properties["lastDeviceId"] == ""
    assert properties["lastConfigDouble"] == UNDEFINED_LAST_DOUBLE

    await instantiateFromName("CHARLIE", name="testConfigClient",
                              serverId="MDL_SERVER_MOCK")

    properties = await getProperties(
        "MDL_SERVER_MOCK",
        ["lastClassId", "lastDeviceId", "lastConfigDouble"])
    assert properties["lastClassId"] == "MockingDevice"
    assert properties["lastDeviceId"] == "CHARLIE"
    assert properties["lastConfigDouble"] == 25.0
    await shutdown("CHARLIE")


@pytest.mark.timeout(20)
@pytest.mark.asyncio(loop_scope="module")
async def test_instantiate_device(deviceTest):
    await execute("MDL_SERVER_MOCK", "reset")
    properties = await getProperties(
        "MDL_SERVER_MOCK",
        ["lastClassId", "lastDeviceId", "lastConfigDouble"])
    assert properties["lastClassId"] == ""
    assert properties["lastDeviceId"] == ""
    assert properties["lastConfigDouble"] == UNDEFINED_LAST_DOUBLE

    h = Hash()
    h["deviceId"] = "ALICE"
    h["name"] = "testConfig"
    h["serverId"] = "MDL_SERVER_MOCK"
    await call(MANAGER_ID, "slotInstantiateDevice", h)

    properties = await getProperties(
        "MDL_SERVER_MOCK",
        ["lastClassId", "lastDeviceId", "lastConfigDouble"])
    assert properties["lastClassId"] == "MockingDevice"
    assert properties["lastDeviceId"] == "ALICE"
    assert properties["lastConfigDouble"] == -1.0

    h = Hash()
    h["deviceId"] = "ALICE"
    h["name"] = "testConfig"
    h["classId"] = "NoClass"
    h["serverId"] = "MDL_SERVER"
    # ClassId missmatch ...
    with pytest.raises(KaraboError):
        await call(MANAGER_ID, "slotInstantiateDevice", h)

    # Name key missing
    with pytest.raises(KaraboError):
        h = Hash()
        h["deviceId"] = "ALICE"
        h["serverId"] = "MDL_SERVER"
        await call(MANAGER_ID, "slotInstantiateDevice", h)


def test_hash_to_hash():
    h = Hash()
    h["bool"] = True
    h["int"] = 4
    h["string"] = "bla ä < / \\ \"' > ]]> & <![CDATA[ =_! (|}"
    h["complex"] = complex(1.0, 0.42)
    h["stringlist"] = ["bla", "blub"]
    h["chars"] = b"bla"
    h["emptystringlist"] = []
    h["char"] = HashByte("c")
    h["none"] = None
    h["vector"] = np.arange(7, dtype=np.int64)
    h["emptyvector"] = np.array([])
    h["vectorbool"] = np.array([True, False, True])
    h["hash"] = Hash("a", 3, "b", 7.1)
    h["hashlist"] = [Hash("a", 3), Hash()]
    h["emptyhashlist"] = HashList()
    sh = Hash()
    sh["a"] = Hash()
    h["schema"] = Schema("blub", hash=sh)

    # Nest hashes
    nested = h.deepcopy()
    assert nested.fullyEqual(h)
    h["nested"] = nested
    r = hashToHash(h)
    assert r.fullyEqual(h)

    # Now with attributes
    h["bool", "bool"] = False
    h["int", "float"] = 7.3
    h["hash", "int"] = 3
    h["string", "chars"] = b"blub"
    h["chars", "string"] = "laber &quot; ö \\ \"' ]]> <![CDATA[ (|}"
    h["vector", "complex"] = complex(1.0, 2.0)
    r = hashToHash(h)
    assert not r.fullyEqual(h)
    attrs = r["vector", ...]
    assert not attrs  # i.e. empty
    attrs = r["nested.vector", ...]
    assert not attrs
