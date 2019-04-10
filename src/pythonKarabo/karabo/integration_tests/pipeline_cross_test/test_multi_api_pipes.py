from datetime import datetime
from time import sleep, time

from karabo.integration_tests.utils import BoundDeviceTestCase
from karabo.bound import Hash, State


class TestCrossPipelining(BoundDeviceTestCase):
    _max_timeout = 60    # in seconds
    _sender_freq = 10.0  # in Hz
    _fast_proc_time = 0  # fast receiver processing time (ms.)
    _slow_proc_time = 500   # slow receiver processing time (ms.)
    _test_duration = 5  # in seconds

    def test_1to1_wait_fastReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        self._test_1to1_wait_fastReceiver("cpp", "cpp")
        self._test_1to1_wait_fastReceiver("cpp", "bound")
        # self._test_1to1_wait_fastReceiver("cpp", "mdl")
        self._test_1to1_wait_fastReceiver("bound", "bound")
        self._test_1to1_wait_fastReceiver("bound", "cpp")
        # self._test_1to1_wait_fastReceiver("bound", "mdl")
        # self._test_1to1_wait_fastReceiver("mdl", "cpp")
        # self._test_1to1_wait_fastReceiver("mdl", "bound")

    def test_1to1_wait_slowReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        self._test_1to1_wait_slowReceiver("cpp", "cpp")
        self._test_1to1_wait_slowReceiver("cpp", "bound")
        # self._test_1to1_wait_slowReceiver("cpp", "mdl")
        self._test_1to1_wait_slowReceiver("bound", "bound")
        self._test_1to1_wait_slowReceiver("bound", "cpp")
        # self._test_1to1_wait_slowReceiver("bound", "mdl")
        # self._test_1to1_wait_slowReceiver("mdl", "cpp")
        # self._test_1to1_wait_slowReceiver("mdl", "bound")

    def test_1to1_queue_fastReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        self._test_1to1_queue_fastReceiver("cpp", "cpp")
        self._test_1to1_queue_fastReceiver("cpp", "bound")
        # self._test_1to1_queue_fastReceiver("cpp", "mdl")
        self._test_1to1_queue_fastReceiver("bound", "bound")
        self._test_1to1_queue_fastReceiver("bound", "cpp")
        # self._test_1to1_queue_fastReceiver("bound", "mdl")
        # self._test_1to1_queue_fastReceiver("mdl", "cpp")
        # self._test_1to1_queue_fastReceiver("mdl", "bound")

    def test_1to1_queue_slowReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        self._test_1to1_queue_slowReceiver("cpp", "cpp")
        self._test_1to1_queue_slowReceiver("cpp", "bound")
        # self._test_1to1_queue_slowReceiver("cpp", "mdl")
        self._test_1to1_queue_slowReceiver("bound", "bound")
        self._test_1to1_queue_slowReceiver("bound", "cpp")
        # self._test_1to1_queue_slowReceiver("bound", "mdl")
        # self._test_1to1_queue_slowReceiver("mdl", "cpp")
        # self._test_1to1_queue_slowReceiver("mdl", "bound")

    def test_1to1_drop_fastReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        self._test_1to1_drop_fastReceiver("cpp", "cpp")
        self._test_1to1_drop_fastReceiver("cpp", "bound")
        # self._test_1to1_drop_fastReceiver("cpp", "mdl")
        self._test_1to1_drop_fastReceiver("bound", "bound")
        self._test_1to1_drop_fastReceiver("bound", "cpp")
        # self._test_1to1_drop_fastReceiver("bound", "mdl")
        # self._test_1to1_drop_fastReceiver("mdl", "cpp")
        # self._test_1to1_drop_fastReceiver("mdl", "bound")

    def test_1to1_drop_slowReceiver(self):
        # Start all servers you need in the end:
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        # self.start_server_num("mdl", 1)

        self._test_1to1_drop_slowReceiver("cpp", "cpp")
        self._test_1to1_drop_slowReceiver("cpp", "bound")
        # self._test_1to1_drop_slowReceiver("cpp", "mdl")
        self._test_1to1_drop_slowReceiver("bound", "bound")
        self._test_1to1_drop_slowReceiver("bound", "cpp")
        # self._test_1to1_drop_slowReceiver("bound", "mdl")
        # self._test_1to1_drop_slowReceiver("mdl", "cpp")
        # self._test_1to1_drop_slowReceiver("mdl", "bound")

    def _test_1to1_wait_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender wait.
        """
        self._test_1to1_wait(sender_api, self._sender_freq, receiver_api, self._fast_proc_time)

    def _test_1to1_wait_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender wait.
        """
        self._test_1to1_wait(sender_api, self._sender_freq, receiver_api, self._slow_proc_time)

    def _test_1to1_queue_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender queue.
        """
        self._test_1to1_queue(sender_api, self._sender_freq, receiver_api, self._fast_proc_time)

    def _test_1to1_queue_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender queue.
        """
        self._test_1to1_queue(sender_api, self._sender_freq, receiver_api, self._slow_proc_time)

    def _test_1to1_drop_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender drop.
        """
        self._test_1to1_drop(sender_api, self._sender_freq, receiver_api, self._fast_proc_time)

    def _test_1to1_drop_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender drop.
        """
        self._test_1to1_drop(sender_api, self._sender_freq, receiver_api, self._slow_proc_time)

    def _test_1to1_wait(self, sender_api, sender_freq,
                        receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        wait.
        """
        sender_cfg = Hash("outputFrequency", sender_freq)
        self.start_device(sender_api, 1, "sender", sender_cfg)

        receiver_cfg = Hash("input.connectedOutputChannels", "sender:output",
                            "input.onSlowness", "wait",
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(self._test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()

        elapsed_time = stop_time - start_time

        self.assertTrue(self.waitUntilEqual("receiver", "state",
                                            State.NORMAL, self._max_timeout),
                                            "'{}' Receiver didn't reach NORMAL state within {} secs. after sender stop."
                                            .format(receiver_api, self._max_timeout))
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +/-20%:
        cycle_time = max(1.0/self._sender_freq, processing_time/1000.0)  # for wait, processing_time "holds" the sender.
        expected_out_count = elapsed_time / cycle_time
        min_expected = 0.80 * expected_out_count
        max_expected = 1.20 * expected_out_count
        self.assertTrue(min_expected < out_count < max_expected,
                        "# of output data items, {}, is not in the expected interval, ({:.2f}, {:.2f})."
                        .format(out_count, min_expected, max_expected))

        # Could still take a while until all data is received
        self.assertTrue(self.waitUntilEqual("receiver", "inputCounter",
                                            out_count, self._max_timeout),
                                            "'{}' Input ({}) and '{}' Output ({}) counters didn't converge within {} secs."
                                            .format(receiver_api, self.dc.get("receiver", "inputCounter"),
                                                    sender_api, out_count, self._max_timeout))

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, "Problem killing sender device: '{}'.".format(msg))

        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, "Problem killing receiver device: '{}'.".format(msg))

    def _test_1to1_queue(self, sender_api, sender_freq,
                         receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        queue.
        """
        sender_cfg = Hash("outputFrequency", sender_freq)
        self.start_device(sender_api, 1, "sender", sender_cfg)

        receiver_cfg = Hash("input.connectedOutputChannels", "sender:output",
                            "input.onSlowness", "queue",
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(self._test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()

        elapsed_time = stop_time - start_time

        self.assertTrue(self.waitUntilEqual("receiver", "state",
                                            State.NORMAL, self._max_timeout),
                                            "'{}' Receiver didn't reach NORMAL state within {} secs. after sender stop."
                                            .format(receiver_api, self._max_timeout))
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +/-20%:
        min_expected = 0.80 * sender_freq * elapsed_time
        max_expected = 1.20 * sender_freq * elapsed_time
        self.assertTrue(min_expected < out_count < max_expected,
                        "# of output data items, {}, is not in the expected interval, ({:.2f}, {:.2f})."
                        .format(out_count, min_expected, max_expected))

        # Could still take a while until all data is received
        self.assertTrue(self.waitUntilEqual("receiver", "inputCounter",
                                            out_count, self._max_timeout),
                                            "'{}' Input ({}) and '{}' Output ({}) counters didn't converge within {} secs."
                                            .format(receiver_api, self.dc.get("receiver", "inputCounter"),
                                                    sender_api, out_count, self._max_timeout))

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, "Problem killing sender device: '{}'.".format(msg))

        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, "Problem killing receiver device: '{}'.".format(msg))

    def _test_1to1_drop(self, sender_api, sender_freq,
                        receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        drop.
        """
        sender_cfg = Hash("outputFrequency", sender_freq)
        self.start_device(sender_api, 1, "sender", sender_cfg)

        receiver_cfg = Hash("input.connectedOutputChannels", "sender:output",
                            "input.onSlowness", "drop",
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(self._test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()

        elapsed_time = stop_time - start_time

        self.assertTrue(self.waitUntilEqual("receiver", "state",
                                            State.NORMAL, self._max_timeout),
                                            "'{}' Receiver didn't reach NORMAL state within {} secs. after sender stop."
                                            .format(receiver_api, self._max_timeout))
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +/-20%:
        min_expected = 0.80 * sender_freq * elapsed_time
        max_expected = 1.20 * sender_freq * elapsed_time
        self.assertTrue(min_expected < out_count < max_expected,
                        "# of output data items, {}, is not in the expected interval ({:.2f}, {:.2f})."
                        .format(out_count, min_expected, max_expected))

        inputCounter = self.dc.get("receiver", "inputCounter")

        sender_delay_ms = 1.0/sender_freq * 1000
        if (sender_delay_ms - processing_time > 50):
            # Receiver is faster than sender by a large margin (50 ms.). Almost no drop is expected.
            min_received_expected = 0.90 * out_count
            max_received_expected = 1.10 * out_count
            self.assertTrue(min_received_expected < inputCounter < max_received_expected,
                            "# of input data items, {}, is not in the expected interval ({:.2f}, {:.2f})."
                            .format(inputCounter, min_received_expected, max_received_expected))
        else:
            # Receiver is not much faster (or is slower) than the sender. Some drop is expected.
            # Assert that at least some data has arrived.
            self.assertTrue(inputCounter > 0, "At least one data item should have been received.")

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, "Problem killing sender device: '{}'.".format(msg))

        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, "Problem killing receiver device: '{}'.".format(msg))

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
        self.assertTrue(ok,
                        "Could not start device '{}' on server '{}': '{}'."
                        .format(klass, self.serverId(api, server_num), msg))

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
