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
import os
from asyncio import sleep, wait_for
from pathlib import Path
from unittest.mock import AsyncMock
from uuid import uuid4

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Device, Hash, KaraboError, String, call, connectDevice, slot, updateDevice)
from karabo.middlelayer.testing import AsyncDeviceContext, sleepUntil
from karabo.middlelayer_devices.project_manager import ProjectManager
from karabo.project_db.database import SQLDatabase
from karabo.project_db.exist_db import (
    TESTDB_ADMIN_PASSWORD, ExistDatabase, stop_local_database)
from karabo.project_db.testing import create_hierarchy

UUIDS = [str(uuid4()) for i in range(5)]
_PROJECT_DB_TEST = "projManTest"


class ConsumerDevice(Device):
    client = String(
        defaultValue="")

    projectManagerUpdate = String(
        defaultValue="")

    @slot
    async def connectProject(self, instance):
        await self._sigslot.async_connect(instance, "signalProjectUpdate",
                                          self.slotProject)
        return True

    @slot
    async def slotProject(self, info_hash, deviceId):
        client = info_hash.get("client", "")
        self.client = client
        self.projectManagerUpdate = deviceId
        await sleep(0)


_EXIST_DB_LOCAL = Hash(
    "protocol", "exist_db",
    "exist_db", Hash(
        "host", os.getenv('KARABO_TEST_PROJECT_DB', 'localhost'),
        "port", int(os.getenv('KARABO_TEST_PROJECT_DB_PORT', '8181'))),
    "testMode", True)

_LOCAL_DB_NAME = "testDbRemove"
_LOCAL_DB = Hash(
    "protocol", "local",
    "local", Hash("dbName", _LOCAL_DB_NAME)
)


@pytest_asyncio.fixture(loop_scope="module")
async def exist_database():
    _user = "admin"
    _password = TESTDB_ADMIN_PASSWORD
    db_init = ExistDatabase(
        _user, _password, test_mode=True, init_db=True)

    # Create test data
    async with db_init as db:
        path = "{}/{}".format(db.root, 'LOCAL')
        if db.dbhandle.hasCollection(path):
            db.dbhandle.removeCollection(path)
        # make sure we have the LOCAL collection and subcollections
        path = "{}/{}".format(db.root, 'LOCAL')
        if not db.dbhandle.hasCollection(path):
            db.dbhandle.createCollection(path)
        _, device_config_uuid_map = await create_hierarchy(db)
    ret = {"session": db_init, "name": "exist_database",
           "device_config_uuid_map": device_config_uuid_map}
    yield ret

    # Cleanup before stop
    async with db_init as db:
        path = "{}/{}".format(db.root, 'LOCAL')
        if db.dbhandle.hasCollection(path):
            db.dbhandle.removeCollection(path)

    # More cleanup
    ownDir = os.path.dirname(os.path.abspath(__file__))
    dirs = [ownDir, os.path.join(ownDir, '..')]
    for dir in dirs:
        files = os.listdir(dir)
        for file in files:
            if 'openMQLib.log' in file:
                os.remove(os.path.join(dir, file))
            if 'device-projManTest' in file:
                os.remove(os.path.join(dir, file))
            if 'karabo.log' in file:
                os.remove(os.path.join(dir, file))
            if 'serverId.xml' in file:
                os.remove(os.path.join(dir, file))

    stop_local_database()


@pytest_asyncio.fixture(loop_scope="module")
async def exist_db_testcase(exist_database):
    conf = Hash("_deviceId_", _PROJECT_DB_TEST)
    conf["projectDB"] = _EXIST_DB_LOCAL
    local = ProjectManager(conf)
    consume = ConsumerDevice({"_deviceId_": "consumeTest"})
    async with AsyncDeviceContext(local=local, consume=consume):
        yield exist_database


@pytest_asyncio.fixture(loop_scope="module")
async def sql_database():
    folder = Path(os.environ["KARABO"]).joinpath(
        "var", "data", "project_db")
    path = folder / _LOCAL_DB_NAME
    path.parent.mkdir(parents=True, exist_ok=True)

    db_init = SQLDatabase(local=True, db_name=path)
    await db_init.initialize()
    # Create test data
    async with db_init as db:
        _, device_config_uuid_map = await create_hierarchy(db)
    ret = {"session": db_init, "name": "sql_database",
           "device_config_uuid_map": device_config_uuid_map}
    yield ret
    await db_init.delete()


@pytest_asyncio.fixture(loop_scope="module")
async def sql_database_testcase(sql_database):
    conf = Hash("_deviceId_", _PROJECT_DB_TEST)
    conf["projectDB"] = _LOCAL_DB
    local = ProjectManager(conf)
    consume = ConsumerDevice({"_deviceId_": "consumeTest"})
    async with AsyncDeviceContext(local=local, consume=consume):
        yield sql_database


@pytest.fixture
def db_fixture(request):
    return request.getfixturevalue(request.param)


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
@pytest.mark.parametrize("db_fixture",
                         ["exist_db_testcase", "sql_database_testcase"],
                         indirect=True)
async def test_project_manager(db_fixture, subtests, mocker):
    # tests are run in sequence as sub tests
    # device server thus is only instantiated once
    # we allow for sleeps in the integration tests as some messaging
    # is async.
    # Additionally, we borrow the SignalSlotable of the device server
    device_config_uuid_map = db_fixture["device_config_uuid_map"]

    with subtests.test(msg="Test list items"):
        ret = await wait_for(call(
            _PROJECT_DB_TEST, "slotListItems",
            Hash("domain", "LOCAL")), timeout=5)
        items = ret.get('items')
        assert len(items) == 61
        scenecnt = 0
        for i in items:
            if i.get("item_type") == "scene":
                scenecnt += 1
        assert scenecnt == 4

    with subtests.test(msg="Test generic interface"):
        arg = Hash("type", "listItems",
                   "domain", "LOCAL")
        ret = await wait_for(call(
            _PROJECT_DB_TEST, "slotGenericRequest", arg), timeout=5)
        items = ret.get('items')
        assert len(items) == 61
        scenecnt = 0
        for i in items:
            if i.get("item_type") == "scene":
                scenecnt += 1
        assert scenecnt == 4
        # add optional argument. Gets only scenes
        arg["item_types"] = ["scene"]
        ret = await wait_for(call(
            _PROJECT_DB_TEST, "slotGenericRequest", arg), timeout=5)
        items = ret.get('items')
        assert len(items) == 4
        for i in items:
            assert i.get("item_type") == "scene"

    with subtests.test(msg="Test saving data"):
        ret = await wait_for(call(
            "consumeTest", "connectProject", _PROJECT_DB_TEST), timeout=5)
        assert ret
        proxy = await connectDevice("consumeTest")
        uuid = str(uuid4())
        xml = (f'<test uuid="{uuid}" item_type="project" '
               'simple_name="extrasave">foobar</test>')
        item = Hash()
        item["xml"] = xml
        item["uuid"] = uuid
        item["overwrite"] = False
        # Save a project item!
        item["item_type"] = "project"
        item["domain"] = "LOCAL"
        items = [item]
        h = Hash("items", items, "client",  "client-587")

        with pytest.raises(KaraboError):
            ret = await wait_for(call(
                _PROJECT_DB_TEST, "slotSaveItems", h), timeout=5)
        for fmt in [1, 3, 4]:
            h["schema_version"] = fmt
            with pytest.raises(KaraboError):
                ret = await wait_for(call(
                    _PROJECT_DB_TEST, "slotSaveItems", h), timeout=5)
        h["schema_version"] = 2
        ret = await wait_for(call(
                _PROJECT_DB_TEST, "slotSaveItems", h), timeout=5)
        items = ret.get("items")
        item = items[0]
        assert item.get('uuid') == uuid
        assert item.get("success")

        await updateDevice(proxy)
        await sleepUntil(
            lambda: proxy.projectManagerUpdate == _PROJECT_DB_TEST,
            timeout=3)
        assert proxy.projectManagerUpdate == _PROJECT_DB_TEST
        assert proxy.client == "client-587"

    with subtests.test(msg="Test named list items"):
        # in the project hierarchy created by create_hierarchy
        # there should be only one project and is called "Project"
        h = Hash("domain", "LOCAL", "item_type", "project",
                 "simple_name", "Project")
        ret = await wait_for(call(
            _PROJECT_DB_TEST, "slotListNamedItems", h), timeout=5)
        items = ret.get('items')
        assert len(items) == 1

    with subtests.test(msg="Test listProjectsWithDevice"):
        # in the project hierarchy created by create_hierarchy
        # there should be only one project and is called "Project"
        ret = await wait_for(call(
            _PROJECT_DB_TEST, "slotListProjectsWithDevice",
            Hash("domain", "LOCAL", "name", "karabo")), timeout=5)
        items = ret.get('items')
        assert len(items) == 1
        assert items[0]["project_name"] == "Project"

        ret = await wait_for(call(
            _PROJECT_DB_TEST, "slotListProjectsWithDevice",
            Hash("domain", "LOCAL", "name", "NOTHEREAVAILABLE")),
                             timeout=5)
        items = ret.get('items')
        assert len(items) == 0

    with subtests.test(msg="Test get projects and configurations"):
        for device_id, config_uuid in device_config_uuid_map.items():
            ret = await wait_for(call(
                _PROJECT_DB_TEST, "slotListProjectsWithDeviceConfigurations",
                Hash("domain", "LOCAL", "deviceId", device_id)), timeout=5)
            assert list(ret["items"][0]) == ["project_name", "config_uuid"]
            proj = ret["items"][0]["project_name"]
            conf = ret["items"][0]["config_uuid"]
            assert proj == "Project"
            assert config_uuid == conf

    if db_fixture["name"] == "sql_database":
        with subtests.test(msg="Test instantiate project device"):
            ret = await wait_for(call(
                _PROJECT_DB_TEST, "listDomainWithDevices",
                Hash("domain", "LOCAL")), timeout=5)
            items = ret["items"]
            assert len(items) == 16
            item = items[0]
            for key in ["device_uuid", "device_name",
                        "device_class", "server_name",
                        "project_uuid", "project_name"]:
                assert key in item
            device_uuid = item["device_uuid"]
            assert device_uuid is not None

            instantiate = mocker.patch(
                "karabo.middlelayer_devices.project_manager.instantiate",
                new_callable=AsyncMock)
            h = Hash("deviceId", item["device_name"],
                     "classId", item["device_class"],
                     "serverId", item["server_name"],
                     "device_uuid", item["device_uuid"])
            ret = await wait_for(call(
                _PROJECT_DB_TEST, "instantiateProjectDevice", h), timeout=5)
            kwargs = instantiate.call_args.kwargs
            assert kwargs["classId"] == "a_class"
            assert kwargs["deviceId"] == item["device_name"]
            assert kwargs["serverId"] == item["server_name"]
            assert isinstance(kwargs["configuration"], Hash)
