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

    def test_chain_wait_fastReceiver(self):
        """Checks for pipelines composed of more than 2 devices."""
        self.start_server_num("cpp", 1)
        self.start_server_num("bound", 1)
        self._test_chain_fastReceiver("cpp", 16)
        self._test_chain_fastReceiver("bound", 16)

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

    def _test_chain_fastReceiver(self, api, num_of_forwarders):
        assert(num_of_forwarders > 0)

        start_time = time()

        sender_cfg = Hash("outputFrequency", 10)
        receiver_cfg = Hash("input.connectedOutputChannels", "fwd{}:output".format(num_of_forwarders),
                            "input.onSlowness", "wait",
                            "processingTime", 20)
        start_info = [("sender", sender_cfg), ("receiver", receiver_cfg),
                      ("fwd1", Hash("input.connectedOutputChannels", "sender:output",
                                    "input.onSlowness", "wait",
                                    "processingTime", 20))]

        for i in range(2, num_of_forwarders+1):
            cfg = Hash("input.connectedOutputChannels", f"fwd{i-1}:output",
                       "input.onSlowness", "wait",
                       "processingTime", 20)
            start_info.append((f"fwd{i}", cfg))

        # Instantiates all the devices that will be in the pipeline.
        # The use of instantiateNoWait (from DeviceClient) is required
        # to trigger potential race conditions.
        for devid, cfg in start_info:
            self.start_device_nowait(api, 1, devid, cfg)

        max_waits = min(100 * num_of_forwarders, 450)
        sleep_wait_interv = 0.1

        # Waits for all devices in the chain to show up in the topology.
        num_of_waits = max_waits
        devices_present = set()
        devices_not_present = set([st_inf[0] for st_inf in start_info])
        while num_of_waits > 0 and len(devices_present) < len(start_info):
            devices = self.dc.getDevices()
            for devid in devices_not_present:
                if devid in devices:
                    devices_present.add(devid)
            devices_not_present = devices_not_present - devices_present
            num_of_waits = num_of_waits - 1
            sleep(sleep_wait_interv)

        # If instantiation of any device failed, cleanup by killing the ones that have been instantiated.
        # This is needed for tests with Bound Python devices - during test development, several processes
        # corresponding to python devices in failed test runs weren't killed after the test finished.
        if len(devices_present) < len(start_info) and api == "bound":
            for devid in devices_present:
                self.dc.killDeviceNoWait(devid)

        self.assertTrue(len(devices_present) == len(start_info),
                        "Couldn't instantiate all devices: "
                        "'{}' of '{}' not instantiated."
                        .format(len(devices_not_present), len(start_info)))

        # Waits for all the chained output channels to be properly connected before sending
        # data through the pipe.
        num_of_waits = max_waits
        outputs_connected = set()
        outputs_not_connected = set([st_inf[0] for st_inf in start_info])
        # no connection to check for 'receiver:output'.
        outputs_not_connected.discard('receiver')
        while num_of_waits > 0 and len(outputs_not_connected) > 0:
            for devid in outputs_not_connected:
                try:
                    conns = self.dc.get(devid, 'output.connections')
                except RuntimeError as re:
                    print("Problem retrieving 'output.connections' from '{}': {}".format(devid, re))
                if len(conns) > 0:
                    outputs_connected.add(devid)
            outputs_not_connected = outputs_not_connected - outputs_connected
            num_of_waits = num_of_waits - 1
            sleep(sleep_wait_interv)

        self.assertTrue(len(outputs_not_connected) == 0,
                        "Failed to connect '{}' of the '{}' "
                        "output channels in the chain."
                        .format(len(outputs_not_connected), len(start_info)-1))

        self.dc.execute("sender", "writeOutput")

        out_count = self.dc.get("sender", "outputCounter")

        # Checks that all the forwarders received and passed data along.
        for i in range(num_of_forwarders):
            self.assertTrue(self.waitUntilEqual(f"fwd{i+1}", "outputCounter",
                                                out_count, self._max_timeout),
                            f"fwd{i+1} failed at fowarding data.")

        # Checks that the data reached the final receiver.
        self.assertTrue(self.waitUntilEqual("receiver", "inputCounter",
                                            out_count, self._max_timeout),
                        "Final receiver Input ({}) didn't match sender Output ({})."
                        .format(self.dc.get("receiver", "inputCounter"), out_count))

        # Cleanup the devices used in the test.
        for devid in devices_present:
            ok, msg = self.dc.killDevice(devid, self._max_timeout)
            self.assertTrue(ok, "Problem killing device '{}': '{}'."
                                .format(devid, msg))

        finish_time = time()

        print()
        print("----------------------")
        print("Test for pipeline with '{}' devices for api '{}' succeded in {:.2f} secs."
              .format(len(start_info), api, finish_time-start_time))
        print("----------------------")
        print()

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

        # Test that duration and frequency match by +/-25%:
        cycle_time = max(1.0/self._sender_freq, processing_time/1000.0)  # for wait, processing_time "holds" the sender.
        expected_out_count = elapsed_time / cycle_time
        min_expected = 0.75 * expected_out_count
        max_expected = 1.25 * expected_out_count
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

        # Test that duration and frequency match by +/-25%:
        min_expected = 0.75 * sender_freq * elapsed_time
        max_expected = 1.25 * sender_freq * elapsed_time
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

        # Test that duration and frequency match by +/-25%:
        min_expected = 0.75 * sender_freq * elapsed_time
        max_expected = 1.25 * sender_freq * elapsed_time
        self.assertTrue(min_expected < out_count < max_expected,
                        "# of output data items, {}, is not in the expected interval ({:.2f}, {:.2f})."
                        .format(out_count, min_expected, max_expected))

        inputCounter = self.dc.get("receiver", "inputCounter")

        sender_delay_ms = 1.0/sender_freq * 1000
        if (sender_delay_ms - processing_time > 50):
            # Receiver is faster than sender by a large margin (50 ms.). Almost no drop is expected.
            min_received_expected = 0.90 * out_count
            self.assertTrue(min_received_expected < inputCounter <= out_count,
                            "# of input data items, {}, is not in the expected interval ({:.2f}, {:.2f}]."
                            .format(inputCounter, min_received_expected, out_count))
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

    def start_device_nowait(self, api, server_num, dev_id, cfg):
        """
        Start device with id and config on server defined by api and number
        """
        klass = "PropertyTest"
        if api == "mdl":
            klass += "MDL"
        cfg.set("deviceId", dev_id)

        self.dc.instantiateNoWait(self.serverId(api, server_num),
                                  klass, cfg)

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
            res = None
            try:
                res = self.dc.get(devId, propertyName)
            except RuntimeError as re:
                print("Problem retrieving property value: {}".format(re))
            if res == whatItShouldBe:
                return True
            else:
                sleep(.5)
        return False
