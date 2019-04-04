from datetime import datetime
from time import sleep, time

from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.bound import Hash, State

class TestCrossPipelining(BoundDeviceTestCase):
    _max_timeout = 20

    def test_example(self):
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops all processes.
        self.start_server_num("bound", 1)
        self.start_server_num("cpp", 1)
        self.start_server_num("mdl", 1)

        with self.assertRaises(RuntimeError):
            self.start_server_num("invalidApi", 1)

        servers = self.dc.getServers()

        self.assertEqual(len(servers), 3)
        self.assertTrue("boundServer/1" in servers)
        self.assertTrue("cppServer/1" in servers)
        self.assertTrue("mdlServer/1" in servers)

    def test_1to1_wait_fastReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        # First test is not a cross test, so remove later,
        # rest to be implemented.
        # self._test_1to1_wait_fastReceiver("cpp", "cpp")

        self._test_1to1_wait_fastReceiver("cpp", "bound")
        # self._test_1to1_wait_fastReceiver("cpp", "mdl")
        self._test_1to1_wait_fastReceiver("bound", "cpp")
        # self._test_1to1_wait_fastReceiver("bound", "mdl")
        # self._test_1to1_wait_fastReceiver("mdl", "cpp")
        # self._test_1to1_wait_fastReceiver("mdl", "bound")

    def test_1to1_wait_slowReceiver(self):
        # return # FIXME: Does not yet seem to run...

        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        # First test is not a cross test, so remove later,
        # rest to be implemented.
        # self._test_1to1_wait_slowReceiver("cpp", "cpp")
        self._test_1to1_wait_slowReceiver("cpp", "bound")
        # self._test_1to1_wait_slowReceiver("cpp", "mdl")
        self._test_1to1_wait_slowReceiver("bound", "cpp")
        # self._test_1to1_wait_slowReceiver("bound", "mdl")
        # self._test_1to1_wait_slowReceiver("mdl", "cpp")
        # self._test_1to1_wait_slowReceiver("mdl", "bound")

    def _test_1to1_wait_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender wait.
        """
        self._test_1to1_wait(sender_api, 10., receiver_api, 0)

    def _test_1to1_wait_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender wait.
        """
        self._test_1to1_wait(sender_api, 10., receiver_api, 100)

    def _test_1to1_wait(self, sender_api, sender_freq,
                        receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        wait.
        """
        test_duration = 5.  # seconds

        sender_cfg = Hash("outputFrequency", sender_freq)
        self.start_device(sender_api, 1, "sender", sender_cfg)
        
        receiver_cfg = Hash("input.connectedOutputChannels", "sender:output",
                            "input.onSlowness", "wait",
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()
        
        elapsed_time = stop_time - start_time

        self.assertTrue(self.waitUntilEqual("receiver", "state",
                                            State.NORMAL, self._max_timeout))
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +/-15%:
        self.assertTrue(out_count > 0.85 * sender_freq * elapsed_time)
        self.assertTrue(out_count < 1.15 * sender_freq * elapsed_time)

        # Could still take a while until all data is received
        self.assertTrue(self.waitUntilEqual("receiver", "inputCounter",
                                            out_count, self._max_timeout))
        in_count = self.dc.get("receiver", "inputCounter")
        self.assertEqual(out_count, in_count)

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, msg)
        
        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, msg)

    def start_server_num(self, api, server_num):
        """Start server of given api and number"""

        klasses = ["PropertyTest"]
        if api == "mdl":
            klasses[0] += "MDL"
        server_id = self.serverId(api, server_num)
        self.start_server(api, server_id, klasses)

    def start_device(self, api, server_num, dev_id, cfg):
        """
        Start device with id and config on server defined by api and number
        """
        klass = "PropertyTest"
        if api == "mdl":
            klass += "MDL"
        cfg.set("deviceId", dev_id)

        ok, msg = self.dc.instantiate(self.serverId(api, server_num),
                                      klass, cfg, self._max_timeout)
        self.assertTrue(ok, msg)

    def serverId(self, api, num):
        """Server id of server with given api and number"""
        return "{}Server/{}".format(api, num)

    def waitUntilEqual(self, devId, propertyName, whatItShouldBe, timeout):
        """
        Wait until property 'propertyName' of device 'deviceId' equals
        'whatItShouldBe'.
        Try up to 'timeOut' seconds and wait 0.5 seconds between each try.
        """
        start = datetime.now()
        while (datetime.now() - start).seconds < timeout:
            res = self.dc.get(devId, propertyName)
            if res == whatItShouldBe:
                return True
            else:
                sleep(.5)
        return False
