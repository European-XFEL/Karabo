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
import copy

from karabo.bound import Hash, SignalSlotable
from karabo.bound.testing import BoundDeviceTestCase


class TestDeviceServer(BoundDeviceTestCase):
    _max_timeoutMs = 60_000

    def test_instantiate(self):

        serverId = "PropTestServer"
        deviceId = "propTestDevice"
        classId = "PropertyTest"
        self.start_server("bound", serverId, [classId], logLevel="WARN")

        # Need a helper to call slots with arguments:
        sigSlot = SignalSlotable("sigSlotTestServer")
        sigSlot.start()

        # Can instantiate with default config
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

        # Cannot instantiate twice with same deviceId
        ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                  cfg).waitForReply(self._max_timeoutMs)
        self.assertFalse(ok, msg)
        self.assertEqual(msg, f"{deviceId} already instantiated and alive")

        with self.subTest(msg="test logger content"):
            serverId = "testLogServer"
            class_ids = ["SceneProvidingDevice", "StuckLoggerDevice"]

            self.start_server("bound", serverId, class_ids,
                              namespace="karabo.bound_device_test",
                              logLevel="INFO", serverFlags="Development")

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

        with self.subTest(msg="test logger priority roundtrip"):
            (h,) = sigSlot.request(serverId, "slotPing", 1
                                   ).waitForReply(self._max_timeoutMs)
            self.assertEqual(h["log"], "INFO")
            sigSlot.request(serverId, "slotLoggerPriority",
                            "ERROR").waitForReply(self._max_timeoutMs)
            (h,) = sigSlot.request(serverId, "slotPing", 1,
                                   ).waitForReply(self._max_timeoutMs)
            self.assertEqual(h["log"], "ERROR")
            sigSlot.request(serverId, "slotLoggerPriority",
                            "INFO").waitForReply(self._max_timeoutMs)
            (h,) = sigSlot.request(serverId, "slotPing", 1
                                   ).waitForReply(self._max_timeoutMs)
            self.assertEqual(h["log"], "INFO")

        with self.subTest("Test serverFlags"):
            req = sigSlot.request(serverId, "slotPing", 1)
            (h, ) = req.waitForReply(self._max_timeoutMs)
            self.assertEqual(1, h["serverFlags"])

        with self.subTest(msg="test slow init"):
            serverId = "testServerSlow"
            deviceId = "slowDevice"
            classId = "SlowStartDevice"
            timeout = 2
            # A server that waits only 2 seconds for the device to come up
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

        with self.subTest(msg="test invalid device"):
            # Checks that an invalid device class (with a broken import) fails
            # to load and the loading failure is properly logged.
            serverId = "testInvalidDeviceServer"
            deviceId = "invalidDevice"
            classId = "InvalidImportDevice"

            self.start_server("bound", serverId, [classId],
                              namespace="karabo.bound_device_test",
                              logLevel="INFO", skip_plugins_check=True)

            # Wait for the device server to be available in the topology before
            # querying its log entries.
            server_available = self.waitUntilTrue(
                # Server available in topology
                lambda: serverId in self.dc.getServers(),
                # within 5 seconds
                5)
            self.assertTrue(server_available)

            info = Hash("logs", 100)
            req = sigSlot.request(serverId, "slotLoggerContent", info)
            (cached_log, ) = req.waitForReply(self._max_timeoutMs)
            n_msgs = len(cached_log["content"])
            self.assertTrue(n_msgs > 0)
            # The log entry reporting the error should be to the end of the
            # device server's log.
            error_msg_found = False
            idx = n_msgs - 1
            while not error_msg_found and idx > -1:
                error_msg_found = (
                    "Failure while building schema for class "
                    "InvalidImportDevice" in
                    cached_log["content"][idx]["message"])
                idx -= 1
            self.assertTrue(error_msg_found)
