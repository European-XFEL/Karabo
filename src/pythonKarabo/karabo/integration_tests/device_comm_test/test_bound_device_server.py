import copy
from time import sleep

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

        # Cannot instantiate with first deviceId again
        # Caveat:
        # The slot to start the first incarnation already returned,
        # but the device might still be in init phase. To be FIXED!
        # So we have to wait here until the device really appears...
        ok = self.waitUntilDevice(deviceId, self._max_timeoutMs/100)
        self.assertTrue(ok,
                        f"'{deviceId}' not appeared in {self.dc.getDevices()}")

        ok, msg = sigSlot.request(serverId, "slotStartDevice",
                                  cfg).waitForReply(self._max_timeoutMs)
        self.assertFalse(ok, msg)
        self.assertEqual(msg, f"{deviceId} already instantiated and alive")

    def waitUntilDevice(self, devId, maxTries):
        """
        Wait until device 'devId' appears online.
        Maximum waiting time is maxTries/10 seconds.
        """
        counter = maxTries
        while counter > 0:
            if devId in self.dc.getDevices():
                return True
            else:
                counter -= 1
            sleep(.1)
        return False
