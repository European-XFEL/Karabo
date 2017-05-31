import os.path as op
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import DeviceClient, EventLoop, Hash
from karabo.common.states import State
from karabo.integration_tests.utils import start_bound_api_server

SERVER_ID = "testServer1"


class TestDeviceDeviceComm(TestCase):
    _timeout = 60  # seconds
    _waitTime = 2  # seconds
    _retries = _timeout//_waitTime

    def setUp(self):
        # Note where we are for later cleanip
        self._ownDir = str(op.dirname(op.abspath(__file__)))

        # Start the EventLoop so that DeviceClient works properly
        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        server_args = ["deviceClasses=CommTestDevice", "visibility=1",
                       "Logger.priority=ERROR"]
        self.serverProcess = start_bound_api_server(SERVER_ID, server_args,
                                                    plugin_dir=self._ownDir)
        self.dc = DeviceClient()

        # wait for plugin to appear
        nTries = 0
        while "CommTestDevice" not in self.dc.getClasses(SERVER_ID):
            sleep(self._waitTime)
            if nTries > self._retries:
                raise RuntimeError("Waiting for plugin to appear timed out")
            nTries += 1

        # we will use two devices communicating with each other.
        config = Hash("Logger.priority", "ERROR",
                      "remote", "testComm2",
                      "deviceId", "testComm1")

        classConfig = Hash("classId", "CommTestDevice",
                           "deviceId", "testComm1",
                           "configuration", config)

        self.dc.instantiate(SERVER_ID, classConfig)

        config2 = Hash("Logger.priority", "ERROR",
                       "remote", "testComm1",
                       "deviceId", "testComm2")

        classConfig2 = Hash("classId", "CommTestDevice",
                            "deviceId", "testComm2",
                            "configuration", config2)

        self.dc.instantiate(SERVER_ID, classConfig2)

        # wait for device to init
        state1 = None
        state2 = None
        nTries = 0
        while state1 != State.NORMAL or state2 != State.NORMAL:
            try:
                state1 = self.dc.get("testComm1", "state")
                state2 = self.dc.get("testComm2", "state")
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
        self._eventLoopThread.join(5)

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
