# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import Hash, State
from karabo.integration_tests.utils import BoundDeviceTestCase

max_timeout = 60    # in seconds


class TestSignalSlotOrder(BoundDeviceTestCase):

    def test_cpp(self):
        self._test_order("cpp")

    def test_mdl(self):
        self._test_order("mdl")

    def test_bound(self):
        self._test_order("bound")

    def _test_order(self, api):

        server_id1 = api + "Server/1"
        server_id2 = api + "Server/2"

        dev_class = "PropertyTest"
        namespace = None
        if api == "mdl":
            dev_class = "MdlOrderTestDevice"
            namespace = "karabo.middlelayer_device_test"
        elif api == "bound":
            dev_class = "BoundOrderTestDevice"
            namespace = "karabo.bound_device_test"

        self.start_server(api, server_id1, [dev_class],
                          namespace=namespace)  # , logLevel="INFO")
        if api == "bound":
            # In bound we anyway have different processes
            server_id2 = server_id1
        else:
            self.start_server(api, server_id2, [dev_class],
                              namespace=namespace)  #, logLevel="INFO")

        # Start devices on different servers to avoid inner-process shortcut
        # and test ordering if broker is involved
        receiver_id = "bob_" + api
        sender_id = "alice_" + api
        numMsg = 10_000  # with 30k, mdl needs > 60 s timeout in waitUntilTrue
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
