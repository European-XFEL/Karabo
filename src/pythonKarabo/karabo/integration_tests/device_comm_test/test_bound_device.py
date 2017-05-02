from multiprocessing import Process
import os
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import EventLoop, Hash, DeviceServer, DeviceClient
from karabo.common.states import State


class TestDeviceDeviceComm(TestCase):
    _timeout = 60  # seconds
    _waitTime = 2  # seconds
    _retries = _timeout//_waitTime

    def runServer(self, config):
        t = Thread(target=EventLoop.work)
        t.start()
        server = DeviceServer(config)  # noqa
        EventLoop.stop()
        t.join()

    def setUp(self):
        # uncomment if ever needing to use local broker
        # os.environ['KARABO_BROKER'] = 'tcp://localhost:7777'

        # set the plugin directory to directory of this file
        # the static .egg-info file located in the test directory
        # assures the the pkg_resources plugin loader will indentify
        # the test device as a valid plugin with an entry point
        self._ownDir = os.path.dirname(os.path.abspath(__file__))
        pluginDir = str(self._ownDir)

        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        config = Hash()
        config.set("serverId", "testServer1")
        config.set("pluginDirectory", pluginDir)
        config.set("pluginNames", "")
        config.set("Logger.priority", "ERROR")
        config.set("visibility", 1)
        config.set("connection", Hash())
        config.set("pluginNamespace", "karabo.bound_device")
        config.set("deviceClasses", [])

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
        config = Hash("Logger.priority", "ERROR",
                      "remote", "testComm2",
                      "deviceId", "testComm1")

        classConfig = Hash("classId", "CommTestDevice",
                           "deviceId", "testComm1",
                           "configuration", config)

        self.dc.instantiate("testServer1", classConfig)

        config2 = Hash("Logger.priority", "ERROR",
                       "remote", "testComm1",
                       "deviceId", "testComm2")

        classConfig2 = Hash("classId", "CommTestDevice",
                            "deviceId", "testComm2",
                            "configuration", config2)

        self.dc.instantiate("testServer1", classConfig2)

        # wait for device to init
        state1 = None
        state2 = None
        nTries = 0
        while state1 != State.NORMAL or state2 != State.NORMAL:
            try:
                state1 = self.dc.get("testComm1", "state")
                state2 = self.dc.get("testComm2", "state")
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
        self.dc.killServer("testServer1")
        self.serverProcess.join()
        EventLoop.stop()
        self._eventLoopThread.join(5)
        # delete any log files
        # handles both the case where we started as part of integration
        # tests, or as a single test
        dirs = [self._ownDir, os.path.join(self._ownDir, '..')]
        for cdir in dirs:
            files = os.listdir(cdir)
            for cfile in files:
                if 'openMQLib.log' in cfile:
                    os.remove(os.path.join(cdir, cfile))
                if 'device-testComm' in cfile:
                    os.remove(os.path.join(cdir, cfile))
                if 'karabo.log' in cfile:
                    os.remove(os.path.join(cdir, cfile))
                if 'serverId.xml' in cfile:
                    os.remove(os.path.join(cdir, cfile))

    def test_in_sequence(self):
        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
        # we allow for sleeps in the integration tests as some messaging
        # is async.
        with self.subTest(msg="Test execute slots"):
            self.dc.execute("testComm1", "slotWithoutArguments")
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")

        with self.subTest(msg="Test emit without arguments"):
            self.dc.execute("testComm2", "slotEmitToSlotWithoutArgs")
            sleep(3)
            res = self.dc.get("testComm2", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")

        with self.subTest(msg="Test emit with arguments"):
            self.dc.execute("testComm2", "slotEmitToSlotWithArgs")
            sleep(3)
            res = self.dc.get("testComm2", "someString")
            self.assertEqual(res, "foo")

        with self.subTest(msg="Test request-reply"):
            self.dc.execute("testComm1", "slotRequestArgs")
            sleep(3)
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "one")

        with self.subTest(msg="Test call"):
            self.dc.execute("testComm2", "slotCallSomething")
            sleep(3)
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")
