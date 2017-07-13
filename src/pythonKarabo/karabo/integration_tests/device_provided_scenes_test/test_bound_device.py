import os.path as op
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import DeviceClient, EventLoop, Hash
from karabo.common.api import Capabilities, State
from karabo.integration_tests.utils import start_bound_api_server

SERVER_ID = "testServerSceneProviders"


class TestDeviceProvidedScenes(TestCase):
    _timeout = 60  # seconds
    _waitTime = 2  # seconds
    _retries = _timeout//_waitTime

    def setUp(self):
        self._ownDir = op.dirname(op.abspath(__file__))

        # Start the EventLoop so that DeviceClient works properly
        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        server_args = [
            "deviceClasses=SceneProvidingDevice,NonSceneProvidingDevice",
            "visibility=1",
            "Logger.priority=ERROR"
        ]
        self.serverProcess = start_bound_api_server(SERVER_ID, server_args,
                                                    plugin_dir=self._ownDir)
        self.dc = DeviceClient()

        def classesLoaded():
            classes = self.dc.getClasses(SERVER_ID)
            scenecls_present = "SceneProvidingDevice" in classes
            nonscenecls_present = "NonSceneProvidingDevice" in classes
            return scenecls_present and nonscenecls_present

        # wait for plugin to appear
        nTries = 0
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

        self.dc.instantiate(SERVER_ID, classConfig)

        # we will use two devices communicating with each other.
        config2 = Hash("Logger.priority", "ERROR",
                       "deviceId", "testNoSceneProvider")

        classConfig2 = Hash("classId", "NonSceneProvidingDevice",
                            "deviceId", "testNoSceneProvider",
                            "configuration", config2)

        self.dc.instantiate(SERVER_ID, classConfig2)

        # wait for device to init
        state1 = None
        state2 = None
        nTries = 0
        while state1 != State.NORMAL or state2 != State.NORMAL:
            try:
                state1 = self.dc.get("testSceneProvider", "state")
                state2 = self.dc.get("testNoSceneProvider", "state")
            # A RuntimeError will be raised up to device init
            except RuntimeError:
                sleep(2)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for device to init timed out")
                nTries += 1

    def tearDown(self):
        # Stop the server
        self.serverProcess.terminate()
        # Stop the event loop
        EventLoop.stop()
        self._eventLoopThread.join()

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
