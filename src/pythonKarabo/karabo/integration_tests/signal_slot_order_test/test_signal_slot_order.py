from unittest import skipIf

from karabo.bound import Hash, State
from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.middlelayer_api.broker import jms

max_timeout = 60    # in seconds


class TestSignalSlotOrder(BoundDeviceTestCase):

    @skipIf(not jms, reason="Not supported yet")
    def test_cpp(self):
        self._test_order("cpp")

    def test_mdl(self):
        self._test_order("mdl")

    def _test_order(self, api):

        server_id1 = api + "Server/1"
        server_id2 = api + "Server/2"

        dev_class = "PropertyTest"
        if api == "mdl":
            dev_class += "MDL"

        self.start_server(api, server_id1,
                          [dev_class])  # , logLevel="INFO")
        self.start_server(api, server_id2,
                          [dev_class])  # , logLevel="INFO")

        # Start devices on different servers to avoid inner-process shortcut
        # and test ordering if broker is involved
        receiver_id = "bob_" + api
        sender_id = "alice_" + api
        numMsg = 10000  # with 30k, mdl needs > 60 sec timeout in waitUntilTrue
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
