# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from time import sleep

from karabo.bound import Hash
from karabo.common.api import Capabilities, State
from karabo.integration_tests.utils import BoundDeviceTestCase

SERVER_ID = "testServerSceneProviders"


class TestDeviceProvidedScenes(BoundDeviceTestCase):
    def setUp(self):
        super().setUp()
        class_ids = ['SceneProvidingDevice', 'NonSceneProvidingDevice']
        self.start_server("bound", SERVER_ID, class_ids,
                          namespace="karabo.bound_device_test")

    def test_in_sequence(self):
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops Python processes.
        config = Hash("Logger.priority", "ERROR",
                      "deviceId", "testSceneProvider")

        classConfig = Hash("classId", "SceneProvidingDevice",
                           "deviceId", "testSceneProvider",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig, 30)
        self.assertTrue(ok, msg)

        config2 = Hash("Logger.priority", "ERROR",
                       "deviceId", "testNoSceneProvider")

        classConfig2 = Hash("classId", "NonSceneProvidingDevice",
                            "deviceId", "testNoSceneProvider",
                            "configuration", config2)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig2, 30)
        self.assertTrue(ok, msg)

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
                sleep(self._waitTime)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for device to init timed out")
                nTries += 1

        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
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
