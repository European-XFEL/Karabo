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
from uuid import uuid4

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Device, Hash, String, call, connectDevice, slot, updateDevice)
from karabo.middlelayer.testing import (  # noqa
    AsyncDeviceContext, event_loop_policy, sleepUntil)
from karabo.middlelayer_devices.project_manager import ProjectManager
from karabo.project_db.exist_db import (
    TESTDB_ADMIN_PASSWORD, ExistDatabase, stop_local_database)
from karabo.project_db.testing import create_hierarchy

UUIDS = [str(uuid4()) for i in range(5)]


class ConsumerDevice(Device):
    client = String(
        defaultValue="")

    projectManagerUpdate = String(
        defaultValue="")

    @slot
    async def connectProject(self, instance):
        await self._ss.async_connect(instance, "signalProjectUpdate",
                                     self.slotProject)
        return True

    @slot
    async def slotProject(self, info_hash, deviceId):
        client = info_hash.get("client", "")
        self.client = client
        self.projectManagerUpdate = deviceId
        await sleep(0)


@pytest_asyncio.fixture(loop_scope="module")
async def database():
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
        _, device_id_conf_map = await create_hierarchy(db)
    db_init.device_id_conf_map = device_id_conf_map
    ret = {"session": db_init, "device_id_conf_map": device_id_conf_map}
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
async def deviceTest(database):
    host = os.getenv('KARABO_TEST_PROJECT_DB', 'localhost')
    port = int(os.getenv('KARABO_TEST_PROJECT_DB_PORT', '8181'))
    conf = Hash("_deviceId_", "projManTest")
    conf["projectDB"] = Hash("protocol", "exist_db",
                             "exist_db", Hash("host", host,
                                              "port", int(port)),
                             "testMode", True)
    local = ProjectManager(conf)
    consume = ConsumerDevice({"_deviceId_": "consumeTest"})
    async with AsyncDeviceContext(local=local, consume=consume):
        yield database


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_project_manager(deviceTest, subtests):
    # tests are run in sequence as sub tests
    # device server thus is only instantiated once
    # we allow for sleeps in the integration tests as some messaging
    # is async.
    # Additionally, we borrow the SignalSlotable of the device server
    device_id_conf_map = deviceTest["device_id_conf_map"]
    with subtests.test(msg="Test initializing user session"):
        ret = await wait_for(call("projManTest", "slotBeginUserSession",
                                  "admin"), timeout=5)
        assert ret["success"]

    with subtests.test(msg="Test list items"):
        ret = await wait_for(call("projManTest",
                                  "slotListItems", 'admin',
                                  "LOCAL"), timeout=5)
        assert ret.get("success")
        assert ret.get("reason", "no reason") == ""
        items = ret.get('items')
        assert len(items) == 57
        scenecnt = 0
        for i in items:
            if i.get("item_type") == "scene":
                scenecnt += 1
        assert scenecnt == 4

    with subtests.test(msg="Test generic interface"):
        arg = Hash("type", "listItems",
                   "token", "admin",
                   "domain", "LOCAL")
        ret = await wait_for(call(
            "projManTest", "slotGenericRequest", arg), timeout=5)
        assert ret.get("success")
        assert ret.get("reason", "no reason") == ""
        items = ret.get('items')
        assert len(items) == 57
        scenecnt = 0
        for i in items:
            if i.get("item_type") == "scene":
                scenecnt += 1
        assert scenecnt == 4
        # add optional argument. Gets only scenes
        arg["item_types"] = ["scene"]
        ret = await wait_for(call(
            "projManTest", "slotGenericRequest", arg), timeout=5)
        assert ret.get("success")
        assert ret.get("reason", "no reason") == ""
        items = ret.get('items')
        assert len(items) == 4
        for i in items:
            assert i.get("item_type") == "scene"

    with subtests.test(msg="Test saving data"):
        ret = await wait_for(call(
            "consumeTest", "connectProject", "projManTest"), timeout=5)
        assert ret
        proxy = await connectDevice("consumeTest")
        uuid = str(uuid4())
        xml = f'<test uuid="{uuid}">foobar</test>'
        item = Hash()
        item["xml"] = xml
        item["uuid"] = uuid
        item["overwrite"] = False
        # Save a project item!
        item["item_type"] = "project"
        item["domain"] = "LOCAL"
        items = [item, ]
        ret = await wait_for(call(
            "projManTest", "slotSaveItems", 'admin', items, "client-587"),
                             timeout=5)
        items = ret.get("items")
        item = items[0]
        assert item.get('entry.uuid') == uuid
        assert item.get("success")

        await updateDevice(proxy)
        await sleepUntil(
            lambda: proxy.projectManagerUpdate == "projManTest",
            timeout=3)
        assert proxy.projectManagerUpdate == "projManTest"
        assert proxy.client == "client-587"

    with subtests.test(msg="Test named list items"):
        # in the project hierarchy created by create_hierarchy
        # there should be only one project and is called "Project"
        ret = await wait_for(call(
            "projManTest", "slotListNamedItems", 'admin', "LOCAL",
            "project", "Project"), timeout=5)
        items = ret.get('items')
        assert len(items) == 1

    with subtests.test(msg="Test get projects and configurations"):
        for device_id, conf_id in device_id_conf_map.items():
            ret = await wait_for(call("projManTest",
                                      "slotListProjectAndConfForDevice",
                                      'admin',
                                      "LOCAL",
                                      device_id,
                                      ), timeout=5)

            assert ret["success"]
            proj = ret["items"][0]["project_name"]
            conf = ret["items"][0]["active_config_ref"]
            assert proj == "Project"
            assert conf_id == conf
