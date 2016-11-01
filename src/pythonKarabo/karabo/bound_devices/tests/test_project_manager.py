import os
from unittest import TestCase
from threading import Thread
from time import sleep

from karabo.bound import EventLoop, Hash, DeviceServer, DeviceClient
from karabo.common.states import State
from karabo.project_db.project_database import ProjectDatabase


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
            xml_reps = ['<test uuid="0">foo</test>',
                        '<test uuid="1">goo</test>',
                        '<test uuid="2">hoo</test>',
                        '<test uuid="3">nope</test>']

            for i, rep in enumerate(xml_reps):
                success, meta = db.save_item('LOCAL', 'testconfig{}'
                                             .format(i), rep)
                self.assertTrue(success)

            # we do this twice to have revision info ready
            for i, rep in enumerate(xml_reps):
                success, meta = db.save_item('LOCAL', 'testconfig{}'
                                             .format(i), rep)
                self.assertTrue(success)

            # get version info for what we inserted
            revisions = []
            for i in range(3):
                path = "{}/LOCAL/testconfig{}"\
                        .format(db.root, i)
                v = db.get_versioning_info(path)
                revisions.append(v['revisions'][-1]['id'])

            xml_serv = """
                      <testserver uuid='testserver_m' list_tag='configs'>
                      <configs>
                      """

            for i in range(3):
                xml_serv += '<configuration uuid="{}" revision="{}"/>'\
                            .format(i, revisions[i])
            xml_serv += " </configs></testserver>"

            success, meta = db.save_item('LOCAL', 'testserver_m', xml_serv)
            self.assertTrue(success)

            # again twice to have a revision number
            success, meta = db.save_item('LOCAL', 'testserver_m', xml_serv)
            self.assertTrue(success)

            # now load again
            res = db.load_multi('LOCAL', xml_serv)
            for i, x in enumerate(xml_reps):
                if i == 3:   # this one should not be in the list
                    continue
                test_xml = db._make_xml_if_needed(x)
                res_xml = db._make_xml_if_needed(res[str(i)])
                self.assertEqual(test_xml.text, res_xml.text)

                self.assertEqual(test_xml.attrib['uuid'],
                                 res_xml.attrib['uuid'])

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

        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        config = Hash()
        config.set("serverId", "testServerProject")

        config.set("pluginDirectory", "")
        config.set("pluginNames", "")
        config.set("Logger.priority", "ERROR")
        config.set("visibility", 1)
        config.set("connection", Hash())
        config.set("pluginNamespace", "karabo.bound_device")
        self.server = DeviceServer(config)

        self._serverThread = Thread(target=self.server.run)
        self._serverThread.daemon = True
        self._serverThread.start()

        self.dc = DeviceClient()

        # wait til the server appears in the system topology
        nTries = 0
        while not self.dc.getSystemTopology().has("server"):
            sleep(self._waitTime)
            if nTries > self._retries:
                raise RuntimeError("Waiting for server to appear timed out")
            nTries += 1

        # wait for plugins to appear
        nTries = 0
        while True:
            try:
                self.server.import_plugin("ProjectManager")
                break
            except RuntimeError:
                sleep(2)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for plugin to load timed out")
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

        self.server.instantiateDevice(classConfig)

        # wait for device to init
        state = None
        nTries = 0
        while state != State.NORMAL:
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

    def tearDown(self):
        # TODO: properly destroy server
        # right now we let the join time out as the server does not
        # always exit correctly -> related to event loop refactoring
        self._serverThread.join(5)
        EventLoop.stop()
        self._eventLoopThread.join(5)

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
            self.server.ss.call("projManTest", "slotInitUserSession",
                                "admin", "karabo")

        with self.subTest(msg="Test saving data"):
            xml = '<test uuid="5">foobar</test>'
            item = Hash()
            item.set("xml", xml)
            item.set("uuid", "testdevice1")
            item.set("overwrite", False)
            item.set("domain", "LOCAL")
            items = [item, ]
            ret = self.server.ss.request("projManTest", "slotSaveItems",
                                         items).waitForReply(5000)
            ret = ret[0]  # returns tuple
            self.assertTrue(ret.has('testdevice1'))
            self.assertTrue(ret.get('testdevice1').get("success"))

        with self.subTest(msg="Test loading data"):
            items = [Hash("uuid", "testconfig0"), Hash("uuid", "testconfig1")]
            ret = self.server.ss.request("projManTest", "slotLoadItems",
                                         "LOCAL", items).waitForReply(5000)
            ret = ret[0]  # returns tuple
            self.assertTrue(ret.has('testconfig0'))
            self.assertTrue(ret.has('testconfig1'))
            cmp = 'v:path="/krb_test/LOCAL/testconfig0">foo</test>'
            self.assertTrue(cmp in ret.get("testconfig0"))
            cmp = 'v:path="/krb_test/LOCAL/testconfig1">goo</test>'
            self.assertTrue(cmp in ret.get("testconfig1"))

        with self.subTest(msg="Test loading multiple data"):
            items = [Hash("uuid", "testserver_m"), ]
            ret = self.server.ss.request("projManTest", "slotLoadItemsAndSubs",
                                         "LOCAL", items).waitForReply(5000)
            ret = ret[0]  # returns tuple
            self.assertTrue(ret.has('testserver_m'))
            self.assertTrue(ret.has('0'))
            self.assertTrue('foo' in ret.get('0'))
            self.assertTrue(ret.has('1'))
            self.assertTrue('goo' in ret.get('1'))
            self.assertTrue(ret.has('2'))
            self.assertTrue('hoo' in ret.get('2'))

        with self.subTest(msg="Test get versioning info"):
            items = [Hash("uuid", "testserver_m"), ]
            ret = self.server.ss.request("projManTest", "slotGetVersionInfo",
                                         "LOCAL", items).waitForReply(5000)
            ret = ret[0]  # returns tuple
            self.assertTrue(ret.has("testserver_m"))
            document = ret.get("testserver_m").get("document")
            self.assertEqual(document, "/db/krb_test/LOCAL/testserver_m")
            revisions = ret.get("testserver_m").get("revisions")
            self.assertGreater(len(revisions), 0)
