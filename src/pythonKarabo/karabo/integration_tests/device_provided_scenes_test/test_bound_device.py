from multiprocessing import Process
import os
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import EventLoop, Hash, DeviceServer, DeviceClient
from karabo.common.capabilities import Capabilities
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
        config.set("serverId", "testServerSceneProviders")
        config.set("pluginDirectory", pluginDir)
        config.set("Logger.priority", "ERROR")
        config.set("visibility", 1)
        config.set("connection", Hash())
        config.set("pluginNamespace", "karabo.bound_device")
        config.set("deviceClasses", ["SceneProvidingDevice",
                                     "NonSceneProvidingDevice"])

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

        # wait for plugin to appear
        nTries = 0

        def classesLoaded():
            classes = self.dc.getClasses("testServerSceneProviders")
            scenecls_present = "SceneProvidingDevice" in classes
            nonscenecls_present = "NonSceneProvidingDevice" in classes
            return scenecls_present and nonscenecls_present

        while not classesLoaded():
            sleep(self._waitTime)
            if nTries > self._retries:
                raise RuntimeError("Waiting for plugin to appear timed out")
            nTries += 1

        # we will use two devices communicating with each other.
        config = Hash("Logger.priority", "ERROR",
                      "deviceId", "testSceneProvider")

        classConfig = Hash("classId", "SceneProvidingDevice",
                           "deviceId", "testSceneProvider",
                           "configuration", config)

        self.dc.instantiate("testServerSceneProviders", classConfig)

        # we will use two devices communicating with each other.
        config2 = Hash("Logger.priority", "ERROR",
                       "deviceId", "testNoSceneProvider")

        classConfig2 = Hash("classId", "NonSceneProvidingDevice",
                            "deviceId", "testNoSceneProvider",
                            "configuration", config2)

        self.dc.instantiate("testServerSceneProviders", classConfig2)

        # wait for device to init
        state1 = None
        state2 = None
        nTries = 0
        while state1 != State.NORMAL or state2 != State.NORMAL:
            try:
                state1 = self.dc.get("testSceneProvider", "state")
                state2 = self.dc.get("testNoSceneProvider", "state")
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
        self.dc.killServer("testServerSceneProviders")
        self.serverProcess.join()
        EventLoop.stop()
        self._eventLoopThread.join()
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
        with self.subTest(msg="Test 'scenesAvailable' in instance info"):
            info = self.dc.getSystemTopology()
            self.assertTrue(info.has("device.testSceneProvider"))
            self.assertTrue(info.has("device.testNoSceneProvider"))
            device = info.get("device")
            self.assertTrue(device.hasAttribute("testSceneProvider",
                                                "capabilities"))
            capabilities = device.getAttribute("testSceneProvider",
                                               "capabilities")
            self.assertEqual(capabilities & Capabilities.PROVIDES_SCENES, 1)

            self.assertTrue(device.hasAttribute("testNoSceneProvider",
                                                "capabilities"))
            capabilities = device.getAttribute("testNoSceneProvider",
                                               "capabilities")
            self.assertEqual(capabilities & Capabilities.PROVIDES_SCENES, 0)
