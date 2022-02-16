import copy

from karabo.bound import Hash, SignalSlotable
from karabo.integration_tests.utils import BoundDeviceTestCase


class TestDeviceServer(BoundDeviceTestCase):
    _max_timeoutMs = 60000

    def test_instantiate(self):

        serverId = "PropTestServer"
        deviceId = "propTestDevice"
        classId = "PropertyTest"
        self.start_server("bound", serverId, [classId])  # logLevel="INFO")

        # Need a helper to call slots with arguments:
        sigSlot = SignalSlotable("sigSlotTestServer")
        sigSlot.start()

        # Can instantiate wit default config
        cfg = Hash("classId", classId,
                   "deviceId", deviceId,
                   "configuration", Hash())
        ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                  cfg).waitForReply(self._max_timeoutMs)
        self.assertTrue(ok, msg)  # msg is failure reason
        self.assertEqual(msg, deviceId)  # msg is deviceId

        # Cannot instantiate with invalid config
        cfg2 = copy.copy(cfg)
        cfg2["deviceId"] = deviceId + "/2"
        cfg2["configuration.int32Property_y"] = 42
        ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                  cfg2).waitForReply(self._max_timeoutMs)
        self.assertFalse(ok, msg)
        self.assertEqual(msg, "Encountered unexpected configuration parameter"
                         ": \"int32Property_y\"")

        ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                  cfg).waitForReply(self._max_timeoutMs)
        self.assertFalse(ok, msg)
        self.assertEqual(msg, f"{deviceId} already instantiated and alive")

        with self.subTest(msg="test logger content"):
            serverId = "testLogServer"
            class_ids = ["SceneProvidingDevice", "StuckLoggerDevice"]
            self.start_server("bound", serverId, class_ids,
                              namespace="karabo.bound_device_test",
                              logLevel="INFO")
            cfg = Hash("classId", "SceneProvidingDevice",
                       "deviceId", "ProperlyLoggingDevice",
                       "configuration", Hash())
            ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                      cfg).waitForReply(self._max_timeoutMs)
            self.assertTrue(ok, msg)
            cfg["deviceId"] = "NotRespondingDevice"
            cfg["classId"] = "StuckLoggerDevice"
            ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                      cfg).waitForReply(self._max_timeoutMs)

            self.assertTrue(ok, msg)
            info = Hash("logs", 100)
            req = sigSlot.request(serverId, "slotLoggerContent", info)
            (reply, ) = req.waitForReply(8000)

            self.assertEqual(serverId, reply["serverId"])
            content = reply["content"]
            self.assertTrue(len(content) <= 100)
            self.assertTrue(len(content) > 0)

            categories = set()
            missing_msg = "Missing Logger Content from 'NotRespondingDevice'"
            missing_msg_found = False
            for entry in content:
                cat = entry['category']
                categories.add(cat)
                if serverId == cat and missing_msg in entry["message"]:
                    missing_msg_found = True

            self.assertIn("ProperlyLoggingDevice", categories)
            self.assertNotIn("NotRespondingDevice", categories)
            self.assertIn(serverId, categories)
            self.assertTrue(missing_msg_found)

        with self.subTest(msg="test slow init"):
            serverId = "testServerSlow"
            deviceId = "slowDevice"
            classId = "SlowStartDevice"
            timeout = 2
            # A server that waits only 3 seconds for the device to come up
            self.start_server("bound", serverId, [classId],
                              namespace="karabo.bound_device_test",
                              instantiationTimeout=timeout)

            # Start a device that blocks in __init__, so intantiation will fail
            cfg = Hash("classId", classId,
                       "deviceId", deviceId,
                       "configuration", Hash("initSleep", 3))
            ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                      cfg).waitForReply(self._max_timeoutMs)
            self.assertFalse(ok, msg)
            expectMsg = "Timeout of instantiation: "
            expectMsg += f"{deviceId} did not confirm it is up "
            expectMsg += f"within {timeout} seconds"
            self.assertEqual(msg, expectMsg)
