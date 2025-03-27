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
from abc import abstractmethod
from asyncio import sleep, wait_for
from uuid import uuid4

from karabo.middlelayer import (
    Device, Hash, String, call, connectDevice, slot, updateDevice)
from karabo.middlelayer.testing import async_tst, sleepUntil
from karabo.project_db.tests.util import create_hierarchy

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


class VerificationProjectManager():
    @abstractmethod
    def _db_init(self):
        """Creates a Database object to inject data in the back-end"""
        raise NotImplementedError

    def stop_local_database(self):
        """Stops the database, overload if needed

        e.g. in the existDb case"""
        pass

    def _createTestData(self):
        with self._db_init() as db:
            path = "{}/{}".format(db.root, 'LOCAL')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

            # make sure we have the LOCAL collection and subcollections
            path = "{}/{}".format(db.root, 'LOCAL')
            if not db.dbhandle.hasCollection(path):
                db.dbhandle.createCollection(path)

            _, self.device_id_conf_map = create_hierarchy(db)

    def _cleanDataBase(self):
        with self._db_init() as db:
            path = "{}/{}".format(db.root, 'LOCAL')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

    def setUp(self):
        # create some test data in the database
        self._createTestData()
        self._ownDir = os.path.dirname(os.path.abspath(__file__))

    def tearDown(self):
        # delete any log files
        # handles both the case where we started as part of integration
        # tests, or as a single test
        dirs = [self._ownDir, os.path.join(self._ownDir, '..')]
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

        self._cleanDataBase()
        self.stop_local_database()

    @async_tst
    async def test_project_manager(self):
        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
        # we allow for sleeps in the integration tests as some messaging
        # is async.
        # Additionally, we borrow the SignalSlotable of the device server

        with self.subTest(msg="Test initializing user session"):
            ret = await wait_for(call("projManTest", "slotBeginUserSession",
                                      "admin"), timeout=5)
            self.assertTrue(ret["success"])

        with self.subTest(msg="Test list items"):
            ret = await wait_for(call("projManTest",
                                      "slotListItems", 'admin',
                                      "LOCAL"), timeout=5)
            self.assertTrue(ret.get("success"), ret.get("reason", "no reason"))
            items = ret.get('items')
            self.assertEqual(len(items), 57)
            scenecnt = 0
            for i in items:
                if i.get("item_type") == "scene":
                    scenecnt += 1
            self.assertEqual(scenecnt, 4)

        with self.subTest(msg="Test generic interface"):
            arg = Hash("type", "listItems",
                       "token", "admin",
                       "domain", "LOCAL")
            ret = await wait_for(call("projManTest",
                                      "slotGenericRequest",
                                      arg),
                                 timeout=5)
            self.assertTrue(ret.get("success"), ret.get("reason", "no reason"))
            items = ret.get('items')
            self.assertEqual(len(items), 57)
            scenecnt = 0
            for i in items:
                if i.get("item_type") == "scene":
                    scenecnt += 1
            self.assertEqual(scenecnt, 4)
            # add optional argument. Gets only scenes
            arg["item_types"] = ["scene"]
            ret = await wait_for(call("projManTest",
                                      "slotGenericRequest",
                                      arg),
                                 timeout=5)
            self.assertTrue(ret.get("success"), ret.get("reason", "no reason"))
            items = ret.get('items')
            self.assertEqual(len(items), 4)
            for i in items:
                self.assertEqual(i.get("item_type"), "scene")

        with self.subTest(msg="Test saving data"):
            ret = await wait_for(call("consumeTest", "connectProject",
                                      "projManTest"), timeout=5)
            self.assertTrue(ret)
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
            ret = await wait_for(call("projManTest", "slotSaveItems",
                                      'admin', items, "client-587"), timeout=5)
            items = ret.get("items")
            item = items[0]
            self.assertEqual(item.get('entry.uuid'), uuid)
            self.assertTrue(item.get("success"))

            await updateDevice(proxy)
            await sleepUntil(
                lambda: proxy.projectManagerUpdate == "projManTest",
                timeout=3)
            self.assertEqual(proxy.projectManagerUpdate, "projManTest")
            self.assertEqual(proxy.client, "client-587")

        with self.subTest(msg="Test named list items"):
            # in the project hierarchy created by create_hierarchy
            # there should be only one project and is called "Project"
            ret = await wait_for(call("projManTest",
                                      "slotListNamedItems", 'admin',
                                      "LOCAL", "project",
                                      "Project"), timeout=5)
            items = ret.get('items')
            self.assertEqual(len(items), 1)

        with self.subTest(msg="Test get projects and configurations"):
            for device_id, conf_id in self.device_id_conf_map.items():
                ret = await wait_for(call("projManTest",
                                          "slotListProjectAndConfForDevice",
                                          'admin',
                                          "LOCAL",
                                          device_id,
                                          ), timeout=5)

                self.assertTrue(ret["success"])
                proj = ret["items"][0]["project_name"]
                conf = ret["items"][0]["active_config_ref"]
                self.assertEqual(proj, "Project")
                self.assertEqual(conf_id, conf)
