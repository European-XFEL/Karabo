import os
from multiprocessing import Process
from threading import Thread
from time import sleep
from unittest import TestCase
from uuid import uuid4

from karabo.bound import (EventLoop, Hash, DeviceServer, DeviceClient,
                          SignalSlotable)
from karabo.common.states import State
from karabo.project_db.project_database import ProjectDatabase
from karabo.project_db.tests.test_projectDatabase import create_hierarchy

UUIDS = [str(uuid4()) for i in range(5)]


class TestProjectManager(TestCase):
    _timeout = 60   # seconds
    _waitTime = 2   # seconds
    _retries = _timeout//_waitTime
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
                success, meta = db.save_item('LOCAL', uuid, rep)
                self.assertTrue(success)

            xml_serv = '<testserver uuid="{0}"><configs>'.format(UUIDS[4])

            for i in range(3):
                xml_serv += '<configuration uuid="{0}" />'.format(UUIDS[i])
            xml_serv += " </configs></testserver>"

            success, meta = db.save_item('LOCAL', UUIDS[4], xml_serv)
            self.assertTrue(success)
            create_hierarchy(db)

    def _cleanDataBase(self):
        with ProjectDatabase(self._user, self._password, server='localhost',
                             test_mode=True) as db:
            path = "{}/{}".format(db.root, 'LOCAL')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

    def runServer(self, config):
        t = Thread(target=EventLoop.work)
        t.start()
        server = DeviceServer(config)  # noqa
        EventLoop.stop()
        t.join()

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

        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.start()

        config = Hash()
        config.set("serverId", "testServerProject")

        config.set("deviceClasses", ["ProjectManager"])
        config.set("Logger.priority", "ERROR")
        config.set("visibility", 1)
        config.set("connection", Hash())
        config.set("pluginNamespace", "karabo.bound_device")

        self.serverProcess = Process(target=self.runServer, args=(config,))
        self.serverProcess.start()
        self.dc = DeviceClient()

        # wait til the server appears in the system topology
        nTries = 0
        while not self.dc.getSystemTopology().has("server"):
            sleep(self._waitTime)
            if nTries > self._retries:
                raise RuntimeError("Waiting for server to appear timed out")
            nTries += 1

        # we will use two devices communicating with each other.
        config = Hash()
        config.set("Logger.priority", "ERROR")
        config.set("deviceId", "projManTest")
        config.set("host", "localhost")
        config.set("port", 8080)
        config.set("testMode", True)
        config.set("classId", "ProjectManager")

        classConfig = Hash("classId", "ProjectManager",
                           "deviceId", "projManTest",
                           "configuration", config)

        self.dc.instantiate("testServerProject", classConfig)

        # wait for device to init
        state = None
        nTries = 0
        while state != State.ON:
            try:
                state = self.dc.get("projManTest", "state")
                break
            # a boost::python error will be thrown up to device init
            # these exceptions are not directly importable, thus we catch
            # very generically here
            except:
                sleep(2)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for device to init timed out")
                nTries += 1

        self.ss = SignalSlotable.create("myTestSigSlot")
        self.ss.start()

    def tearDown(self):
        self.dc.killServer("testServerProject")
        self.serverProcess.join()
        EventLoop.stop()
        self._eventLoopThread.join()

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

    def test_project_manager(self):
        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
        # we allow for sleeps in the integration tests as some messaging
        # is async.
        # Additionally, we borrow the SignalSlotable of the device server

        with self.subTest(msg="Test initializing user session"):
            ret = self.ss.request("projManTest", "slotBeginUserSession",
                                  "admin").waitForReply(5000)
            self.assertTrue(ret[0].get("success"))

        with self.subTest(msg="Test saving data"):
            uuid = str(uuid4())
            xml = '<test uuid="{0}">foobar</test>'.format(uuid)
            item = Hash()
            item.set("xml", xml)
            item.set("uuid", uuid)
            item.set("overwrite", False)
            item.set("domain", "LOCAL")
            items = [item, ]
            ret = self.ss.request("projManTest", "slotSaveItems",
                                  'admin', items).waitForReply(5000)
            items = ret[0].get("items")
            item = items[0]
            self.assertEqual(item.get('entry.uuid'), uuid)
            self.assertTrue(item.get("success"))

        with self.subTest(msg="Test loading data"):
            items = [Hash("uuid", UUIDS[0], "domain", "LOCAL"),
                     Hash("uuid", UUIDS[1], "domain", "LOCAL")]
            ret = self.ss.request("projManTest", "slotLoadItems",
                                  'admin', items).waitForReply(5000)
            ret = ret[0]  # returns tuple
            items = {it['uuid']: it for it in ret['items']}
            self.assertTrue(UUIDS[0] in items)
            self.assertTrue(UUIDS[1] in items)
            cmp = 'v:path="/krb_test/LOCAL/{}">foo</test>'.format(UUIDS[0])
            self.assertTrue(cmp in items[UUIDS[0]].get('xml'))
            cmp = 'v:path="/krb_test/LOCAL/{}">goo</test>'.format(UUIDS[1])
            self.assertTrue(cmp in items[UUIDS[1]].get('xml'))

        with self.subTest(msg="Test list items"):
            queryItems = ['project', 'scene']
            ret = self.ss.request("projManTest",
                                  "slotListItems", 'admin',
                                  "LOCAL",
                                  queryItems).waitForReply(5000)
            items = ret[0].get('items')
            self.assertEqual(len(items), 10)
            scenecnt = 0
            for i in items:
                if i.get("item_type") == "scene":
                    scenecnt += 1
            self.assertEqual(scenecnt, 8)
