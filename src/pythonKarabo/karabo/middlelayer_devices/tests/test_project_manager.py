from asyncio import wait_for
import os
from contextlib import contextmanager
from uuid import uuid4

from karabo.middlelayer import call, Hash
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst
from karabo.project_db.project_database import ProjectDatabase
from karabo.middlelayer_devices.project_manager import ProjectManager
from karabo.project_db.tests.test_projectDatabase import create_hierarchy

UUIDS = [str(uuid4()) for i in range(5)]


class TestProjectManager(DeviceTest):
    _user = "admin"
    _password = "karabo"

    def _createTestData(self):
        with ProjectDatabase(self._user, self._password, server='localhost',
                             test_mode=True) as db:
            # create a device server and multiple config entries
            xml_reps = ['<test uuid="{0}">foo</test>'.format(UUIDS[0]),
                        '<test uuid="{0}">goo</test>'.format(UUIDS[1]),
                        '<test uuid="{0}">hoo</test>'.format(UUIDS[2]),
                        '<test uuid="{0}">nope</test>'.format(UUIDS[3])]

            for rep, uuid in zip(xml_reps, UUIDS):
                meta = db.save_item('LOCAL', uuid, rep)
                self.assertEqual(len(meta), 3)

            xml_serv = '<testserver uuid="{0}"><configs>'.format(UUIDS[4])

            for i in range(3):
                xml_serv += '<configuration uuid="{0}" />'.format(UUIDS[i])
            xml_serv += " </configs></testserver>"

            db.save_item('LOCAL', UUIDS[4], xml_serv)
            create_hierarchy(db)

    def _cleanDataBase(self):
        with ProjectDatabase(self._user, self._password, server='localhost',
                             test_mode=True) as db:
            path = "{}/{}".format(db.root, 'LOCAL')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

    def setUp(self):
        # uncomment if ever needing to use local broker
        # os.environ['KARABO_BROKER'] = 'tcp://localhost:7777'

        # create some test data in the database
        self._createTestData()

        # set the plugin directory to directory of this file
        # the static .egg-info file located in the test directory
        # assures the the pkg_resources plugin loader will indentify
        # the test device as a valid plugin with an entry point
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

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = ProjectManager({"_deviceId_": "projManTest",
                                    "testMode": True})
        with cls.deviceManager(cls.local, lead=cls.local):
            yield

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

        with self.subTest(msg="Test saving data"):
            uuid = str(uuid4())
            xml = '<test uuid="{0}">foobar</test>'.format(uuid)
            item = Hash()
            item["xml"] = xml
            item["uuid"] = uuid
            item["overwrite"] = False
            item["domain"] = "LOCAL"
            items = [item, ]
            ret = await wait_for(call("projManTest", "slotSaveItems",
                                      'admin', items), timeout=5)
            items = ret.get("items")
            item = items[0]
            self.assertEqual(item.get('entry.uuid'), uuid)
            self.assertTrue(item.get("success"))

        with self.subTest(msg="Test list items"):
            ret = await wait_for(call("projManTest",
                                      "slotListItems", 'admin',
                                      "LOCAL"), timeout=5)
            items = ret.get('items')
            self.assertEqual(len(items), 9)
            scenecnt = 0
            for i in items:
                if i.get("item_type") == "scene":
                    scenecnt += 1
            self.assertEqual(scenecnt, 4)
