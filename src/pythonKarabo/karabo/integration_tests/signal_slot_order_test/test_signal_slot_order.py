from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.bound import (Hash, State)

max_timeout = 60    # in seconds


class TestSignalSlotOrder(BoundDeviceTestCase):

    def test_cpp(self):

        api = "cpp"
        server_id1 = api + "Server/1"
        server_id2 = api + "Server/2"
        dev_class = "PropertyTest"  # + "MDL" for mdl API
        self.start_server(api, server_id1,
                          [dev_class])  # , logLevel="INFO")
        self.start_server(api, server_id2,
                          [dev_class])  # , logLevel="INFO")

        # Start devices on different servers to avoid inner-process shortcut
        # and test ordering if broker is involved
        receiver_id = "bob"
        sender_id = "alice"
        numMsg = 30000
        self.start_device(server_id1, dev_class, receiver_id,
                          cfg=Hash("int32Property", numMsg,
                                   "stringProperty", sender_id))
        self.start_device(server_id2, dev_class, sender_id)

        # Now start signal emission and slot calls
        self.dc.execute(receiver_id, "orderTest.slotStart", max_timeout)

        # Wait until all received
        def receiverStateNormal():
            st = self.dc.get(receiver_id, "state")
            return st == State.NORMAL

        self.waitUntilTrue(receiverStateNormal, max_timeout, maxTries=60)
        self.assertEqual(self.dc.get(receiver_id, "state"), State.NORMAL)

        # Finally check results
        testResults = self.dc.get(receiver_id, "orderTest")
        self.assertEqual(testResults["receivedCounts"], numMsg,
                         str(testResults))
        self.assertEqual(testResults["nonConsecutiveCounts"],
                         [0], str(testResults))  # i.e. no disorder

    def start_device(self, server_id, class_id, dev_id, cfg=Hash()):
        """
        Start device with cfg on server
        """
        cfg.set("deviceId", dev_id)

        ok, msg = self.dc.instantiate(server_id, class_id, cfg, max_timeout)
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(class_id, server_id, msg))
