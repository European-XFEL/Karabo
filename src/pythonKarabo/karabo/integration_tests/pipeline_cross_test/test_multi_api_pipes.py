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
from datetime import datetime
from time import sleep, time

from karabo.bound import Hash, SignalSlotable, State
from karabo.bound.testing import BoundDeviceTestCase


class TestCrossPipelining(BoundDeviceTestCase):
    _max_timeout = 60  # in seconds
    _slot_timeout_ms = 5000
    _sender_freq = 20.0  # in Hz
    _fast_proc_time = 0  # fast receiver processing time (ms.)
    _slow_proc_time = 100  # slow receiver processing time (ms.)
    # if 1./sender_freq [in ms] - proc_time > _drop_noDrop_margin,
    # expect that even in drop scenarios nothing is dropped
    _drop_noDrop_margin = 20
    _test_duration = 2.5  # in seconds
    # Note: number of messages written to pipeline per test:
    #       _test_duration * _sender_freq

    def test_1to1(self):
        # Start all servers you need in the end:
        logLevel = 'FATAL'
        self.start_server_num("cpp", 1, logLevel=logLevel)
        self.start_server_num("bound", 1, logLevel=logLevel)
        self.start_server_num("mdl", 1, logLevel=logLevel)

        self._main_test_1to1_wait_fastReceiver()
        self._main_test_1to1_wait_slowReceiver()

        self._main_test_1to1_queueDrop_fastReceiver()
        self._main_test_1to1_queueDrop_slowReceiver()

        self._main_test_1to1_drop_fastReceiver()
        self._main_test_1to1_drop_slowReceiver()

    def _main_test_1to1_wait_fastReceiver(self):

        self._test_1to1_wait_fastReceiver("cpp", "cpp")
        self._test_1to1_wait_fastReceiver("cpp", "bound")
        self._test_1to1_wait_fastReceiver("cpp", "mdl")
        self._test_1to1_wait_fastReceiver("bound", "bound")
        self._test_1to1_wait_fastReceiver("bound", "cpp")
        self._test_1to1_wait_fastReceiver("bound", "mdl")
        self._test_1to1_wait_fastReceiver("mdl", "cpp")
        self._test_1to1_wait_fastReceiver("mdl", "bound")
        self._test_1to1_wait_fastReceiver("mdl", "mdl")

    def _main_test_1to1_wait_slowReceiver(self):

        self._test_1to1_wait_slowReceiver("cpp", "cpp")
        self._test_1to1_wait_slowReceiver("cpp", "bound")
        self._test_1to1_wait_slowReceiver("cpp", "mdl")
        self._test_1to1_wait_slowReceiver("bound", "bound")
        self._test_1to1_wait_slowReceiver("bound", "cpp")
        self._test_1to1_wait_slowReceiver("bound", "mdl")
        self._test_1to1_wait_slowReceiver("mdl", "cpp")
        self._test_1to1_wait_slowReceiver("mdl", "bound")
        self._test_1to1_wait_slowReceiver("mdl", "mdl")

    def _main_test_1to1_queueDrop_fastReceiver(self):

        self._test_1to1_queueDrop_fastReceiver("cpp", "cpp")
        self._test_1to1_queueDrop_fastReceiver("cpp", "bound")
        self._test_1to1_queueDrop_fastReceiver("cpp", "mdl")
        self._test_1to1_queueDrop_fastReceiver("bound", "bound")
        self._test_1to1_queueDrop_fastReceiver("bound", "cpp")
        self._test_1to1_queueDrop_fastReceiver("bound", "mdl")
        self._test_1to1_queueDrop_fastReceiver("mdl", "cpp")
        self._test_1to1_queueDrop_fastReceiver("mdl", "bound")
        self._test_1to1_queueDrop_fastReceiver("mdl", "mdl")

    def _main_test_1to1_queueDrop_slowReceiver(self):

        self._test_1to1_queueDrop_slowReceiver("cpp", "cpp")
        self._test_1to1_queueDrop_slowReceiver("cpp", "bound")
        self._test_1to1_queueDrop_slowReceiver("cpp", "mdl")
        self._test_1to1_queueDrop_slowReceiver("bound", "bound")
        self._test_1to1_queueDrop_slowReceiver("bound", "cpp")
        self._test_1to1_queueDrop_slowReceiver("bound", "mdl")
        self._test_1to1_queueDrop_slowReceiver("mdl", "cpp")
        self._test_1to1_queueDrop_slowReceiver("mdl", "bound")
        self._test_1to1_queueDrop_slowReceiver("mdl", "mdl")

    def _main_test_1to1_drop_fastReceiver(self):

        self._test_1to1_drop_fastReceiver("cpp", "cpp")
        self._test_1to1_drop_fastReceiver("cpp", "bound")
        self._test_1to1_drop_fastReceiver("cpp", "mdl")
        self._test_1to1_drop_fastReceiver("bound", "bound")
        self._test_1to1_drop_fastReceiver("bound", "cpp")
        self._test_1to1_drop_fastReceiver("bound", "mdl")
        self._test_1to1_drop_fastReceiver("mdl", "cpp")
        self._test_1to1_drop_fastReceiver("mdl", "bound")
        self._test_1to1_drop_fastReceiver("mdl", "mdl")

    def _main_test_1to1_drop_slowReceiver(self):

        self._test_1to1_drop_slowReceiver("cpp", "cpp")
        self._test_1to1_drop_slowReceiver("cpp", "bound")
        self._test_1to1_drop_slowReceiver("cpp", "mdl")
        self._test_1to1_drop_slowReceiver("bound", "bound")
        self._test_1to1_drop_slowReceiver("bound", "cpp")
        self._test_1to1_drop_slowReceiver("bound", "mdl")
        self._test_1to1_drop_slowReceiver("mdl", "cpp")
        self._test_1to1_drop_slowReceiver("mdl", "bound")
        self._test_1to1_drop_slowReceiver("mdl", "mdl")

    def test_chain_receivers(self):
        """Checks for pipelines composed of more than 2 devices."""
        # Prepare a list of apis such that we have each API sending to each API
        # NOTE: Total number of cpp must not exceed 64 which is maximum number
        # of PropertyTest devices in one C++ process due to the size of the
        # static buffer on the Memory class
        print("test_chain_receivers")
        apis = ["cpp", "cpp", "bound", "bound", "mdl", "mdl",
                "cpp", "mdl", "bound"] * 14
        logLevel = 'FATAL'
        if "bound" in apis:
            self.start_server_num("bound", 1, logLevel=logLevel)
        if "cpp" in apis:
            self.start_server_num("cpp", 1, logLevel=logLevel)
        if "mdl" in apis:
            self.start_server_num("mdl", 1, logLevel=logLevel)

        num_of_forwarders = len(apis) - 2  # first/last are sender/receiver
        self.assertTrue(num_of_forwarders > 0)

        start_time = time()

        # Assemble ids and configurations for all elements in the chain
        start_info = [("sender", Hash()),
                      ("fwd1", Hash("input.connectedOutputChannels",
                                    ["sender:output"]))]

        for i in range(2, num_of_forwarders+1):
            cfg = Hash("input.connectedOutputChannels", [f"fwd{i-1}:output"])
            start_info.append((f"fwd{i}", cfg))

        receiver_cfg = Hash("input.connectedOutputChannels",
                            [f"fwd{num_of_forwarders}:output"])
        start_info.append(("receiver", receiver_cfg))

        # Instantiates all the devices that will be in the pipeline.
        # The use of instantiateNoWait (from DeviceClient) is required
        # to trigger potential race conditions.
        for index, (devid, cfg) in enumerate(start_info):
            self.start_device_nowait(apis[index], 1, devid, cfg)
            # Add API to start_info for debugging
            start_info[index] = (devid, cfg, apis[index])

        # Create helper for later - best here where we wait for devices anyway
        caller = SignalSlotable("test_chain_receivers_helper")
        caller.start()

        # Up to three minutes waiting for devices to come (CI failed with one)
        max_waits = min(100 * num_of_forwarders, 1800)
        sleep_wait_interv = 0.1

        # Waits for all devices in the chain to show up in the topology.
        num_of_waits = max_waits
        devices_present = set()
        devices_not_present = {st_inf[0] for st_inf in start_info}
        while num_of_waits > 0 and len(devices_present) < len(start_info):
            devices = self.dc.getDevices()
            for devid in devices_not_present:
                if devid in devices:
                    devices_present.add(devid)
            devices_not_present = devices_not_present - devices_present
            num_of_waits = num_of_waits - 1
            sleep(sleep_wait_interv)

        # If instantiation of any device failed, cleanup by killing the
        # ones that have been instantiated.
        # This is needed for tests with Bound Python devices - during
        # test development, several processes
        # corresponding to python devices in failed test runs weren't
        # killed after the test finished.
        # Also collect logs of servers on which devices failed (seen in CI)
        missing = ""
        logs = ""
        if len(devices_present) < len(start_info):
            failedApis = set()
            for devid, _, api in start_info:
                failedApis.update(api)
                self.dc.killDeviceNoWait(devid)
                if devid in devices_not_present:
                    missing += f"{devid} ({api}), "  # add API as debug info

            for failedApi in failedApis:
                req = caller.request(self.serverId(api, 1),
                                     "slotLoggerContent", Hash("logs", 500))
                res = req.waitForReply(self._max_timeout * 1000)[0]
                for h in res["content"]:
                    logs += str(h) + "\n"

        self.assertTrue(len(devices_present) == len(start_info),
                        f"Couldn't instantiate all {len(start_info)} devices: "
                        f"'{missing[:-2]}' not instantiated.\nLogs:\n {logs}")
        print("========== all instantiated ===============")

        # Waits for all the chained output channels to be properly
        # connected before sending data through the pipe.
        num_of_waits = max_waits
        outputs_connected = set()
        outputs_not_connected = {st_inf[0] for st_inf in start_info}
        # no connection to check for 'receiver:output'.
        outputs_not_connected.discard('receiver')
        while num_of_waits > 0 and len(outputs_not_connected) > 0:
            for devid in outputs_not_connected:
                req = caller.request(devid, "slotGetConfigurationSlice",
                                     Hash("paths", ["output.connections"]))
                try:
                    (cfgSlice, ) = req.waitForReply(self._slot_timeout_ms)
                except Exception as e:
                    if num_of_waits < 10:  # Only print last attempts...
                        print("Problem retrieving 'output.connections' from "
                              f"'{devid}': {str(e)}")
                else:
                    if len(cfgSlice['output.connections']) > 0:
                        outputs_connected.add(devid)
            outputs_not_connected = outputs_not_connected - outputs_connected
            num_of_waits = num_of_waits - 1
            sleep(sleep_wait_interv)

        self.assertEqual(len(outputs_not_connected), 0,
                         f"Failed to connect {outputs_not_connected} of the "
                         f"{len(start_info)-1} output channels in the chain")
        print("========== all connected ===============")

        self.dc.execute("sender", "writeOutput")

        out_count = self.dc.get("sender", "outputCounter")

        def checkForwardersProp(propName, propValue, message):
            for i in range(num_of_forwarders):
                self.assertTrue(self.waitUntilEqual(
                    f"fwd{i+1}", propName,
                    propValue, self._max_timeout),
                    f"fwd{i+1}.{propName} did not become "
                    f"{propValue}: {message}.")

        def checkReceiverProp(propName, propValue, message):
            self.assertTrue(self.waitUntilEqual("receiver", propName,
                                                propValue, self._max_timeout),
                            f"receiver.{propName} did not become {propValue}"
                            f": {message}")

        # Checks that all the forwarders received and passed data along.
        checkForwardersProp("outputCounter", out_count,
                            "i.e. device failed forwarding")
        # Checks that the data reached the final receiver.
        checkReceiverProp("inputCounter", out_count,
                          "i.e. does not match sender output.")

        # Check that no endOfStream received so far
        checkForwardersProp("inputCounterAtEos", 0, "non-zero!")
        checkReceiverProp("inputCounterAtEos",  0, "non-zero!")

        # Now send endOfStream followed by a normal write to see that both
        # (still) propagate
        at_eos_count = out_count
        self.dc.execute("sender", "eosOutput")
        self.dc.execute("sender", "writeOutput")
        out_count = self.dc.get("sender", "outputCounter")

        checkForwardersProp("outputCounter", out_count,
                            "i.e. device failed forwarding")
        checkReceiverProp("inputCounter", out_count,
                          "i.e. does not match sender output.")
        checkForwardersProp("inputCounterAtEos", at_eos_count,
                            "i.e. device did not receive 'endOfStream'")
        checkReceiverProp("inputCounterAtEos", at_eos_count,
                          "i.e. device did not receive 'endOfStream'")

        # Cleanup the devices used in the test.
        for devid in devices_present:
            # Synchronous kill would take ages in case of failure...
            self.dc.killDeviceNoWait(devid)

        finish_time = time()

        print("\n----------------------")
        print(
            "Test for pipeline with '{}' devices mixed of apis '{}' succeded "
            "in {:.2f} secs.".format(len(start_info), set(apis),
                                     finish_time - start_time))
        print("----------------------\n")

        # A little sleep to get devices down before servers are shutdown
        # by tear_down() - interference of device shutting down since told so
        # and server telling devices to go down when servers stops?
        sleep(2)

    def _test_1to1_wait_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender wait.
        """
        self._test_1to1_wait(sender_api, self._sender_freq,
                             receiver_api, self._fast_proc_time)

    def _test_1to1_wait_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender wait.
        """
        self._test_1to1_wait(sender_api, self._sender_freq,
                             receiver_api, self._slow_proc_time)

    def _test_1to1_queueDrop_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender queue.
        """
        self._test_1to1_queueDrop(sender_api, self._sender_freq,
                                  receiver_api, self._fast_proc_time)

    def _test_1to1_queueDrop_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender queue.
        """
        self._test_1to1_queueDrop(sender_api, self._sender_freq,
                                  receiver_api, self._slow_proc_time)

    def _test_1to1_drop_fastReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is slower
        than receiver and receiver is configured to make sender drop.
        """
        self._test_1to1_drop(sender_api, self._sender_freq,
                             receiver_api, self._fast_proc_time)

    def _test_1to1_drop_slowReceiver(self, sender_api, receiver_api):
        """
        Test single sender with single receiver where sender is faster
        than receiver and receiver is configured to make sender drop.
        """
        self._test_1to1_drop(sender_api, self._sender_freq,
                             receiver_api, self._slow_proc_time)

    def _test_1to1_wait(self, sender_api, sender_freq,
                        receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        wait.
        """
        print(f"1to1 wait: {sender_api} ({sender_freq} Hz) ==> "
              f"{receiver_api} ({processing_time} ms processing).")
        sender_cfg = Hash("outputFrequency", sender_freq)
        self.start_device(sender_api, 1, "sender", sender_cfg)

        receiver_cfg = Hash("input.connectedOutputChannels", ["sender:output"],
                            "input.onSlowness", "wait",
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        # wait until receiver is connected
        res = self.waitUntilEqual("receiver", "input.missingConnections", [],
                                  self._max_timeout)
        self.assertTrue(res,
                        "Receiver didn't connect within {self._max_timeout} s")
        # (Only checking "input.missingConnections" not 100% reliable, see
        #  InputChannel::onConnect.)
        assert self.waitUntilOutputKnowsInput("sender", "output",
                                              "receiver:input",
                                              self._max_timeout)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(self._test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()

        elapsed_time = stop_time - start_time

        self.assertTrue(self.waitUntilEqual(
            "sender", "state",
            State.NORMAL, self._max_timeout),
            "'{}' Sender didn't reach NORMAL state within {} secs. after stop."
            .format(sender_api, self._max_timeout))
        # Note for JMS broker: Since state has higher priority, its update
        # might have overtaken the last 'outputCounter' update, so out_count
        #  may rarely appear too small by one?
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +25/-50%:
        # for wait, processing_time "holds" the sender.
        cycle_time = max(1.0/sender_freq, processing_time/1000.0)
        expected_out_count = elapsed_time / cycle_time
        # 0.75 * expected_out_count failed sometimes in CI
        min_expected = 0.5 * expected_out_count
        max_expected = 1.25 * expected_out_count
        self.assertTrue(
            min_expected < out_count < max_expected,
            "# of output data items, {}, is not in "
            "the expected interval, ({:.2f}, {:.2f})."
            .format(out_count, min_expected, max_expected))

        # Could still take a while until all data is received
        self.assertTrue(
            self.waitUntilEqual2("receiver", "inputCounter",
                                 "sender", "outputCounter", self._max_timeout),
                        "'{}' Input ({}) and '{}' Output ({}) counters didn't converge within {} secs."  # noqa
                        .format(receiver_api, self.dc.get("receiver", "inputCounter"),  # noqa
                                sender_api, self.dc.get("sender", "outputCounter"), self._max_timeout))  # noqa

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, f"Problem killing sender device: '{msg}'.")

        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, f"Problem killing receiver device: '{msg}'.")

    def _test_1to1_queueDrop(self, sender_api, sender_freq,
                             receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        queue (and drop if queue full). But here nothing is dropped since
        the configured maximum queue length is longer than all we send.
        """
        print(f"1to1 queueDrop: {sender_api} ({sender_freq} Hz) ==> "
              f"{receiver_api} ({processing_time} ms processing).")
        sender_cfg = Hash("outputFrequency", sender_freq)
        if sender_api == "mdl":
            sender_cfg["output.maxQueueLength"] = 100
        self.start_device(sender_api, 1, "sender", sender_cfg)

        receiver_cfg = Hash("input.connectedOutputChannels", ["sender:output"],
                            "input.onSlowness", "queueDrop",
                            "input.maxQueueLength", 100,
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        # wait until receiver is connected
        res = self.waitUntilEqual("receiver", "input.missingConnections", [],
                                  self._max_timeout)
        self.assertTrue(res,
                        "Receiver didn't connect within {self._max_timeout} s")
        # (Only checking "input.missingConnections" not 100% reliable, see
        #  InputChannel::onConnect.)
        assert self.waitUntilOutputKnowsInput("sender", "output",
                                              "receiver:input",
                                              self._max_timeout)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(self._test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()

        elapsed_time = stop_time - start_time

        self.assertTrue(self.waitUntilEqual("sender", "state",
                                            State.NORMAL, self._max_timeout),
                        "'{}' Sender didn't reach NORMAL state within {} secs. after stop."  # noqa
                        .format(sender_api, self._max_timeout))
        # Note: Since state has higher priority, its update might have
        # overtaken the last
        #       'outputCounter' update, so out_count may rarely appear
        # too small by one?
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +/-25%:
        min_expected = 0.75 * sender_freq * elapsed_time
        max_expected = 1.25 * sender_freq * elapsed_time
        self.assertTrue(
            min_expected < out_count < max_expected,
            "# of output data items, {}, is not in the expected "
            "interval, ({:.2f}, {:.2f})."
            .format(out_count, min_expected, max_expected))

        # Could still take a while until all data is received
        self.assertTrue(self.waitUntilEqual2("receiver", "inputCounter",
                                             "sender", "outputCounter", self._max_timeout),  # noqa
                        "'{}' Input ({}) and '{}' Output ({}) counters didn't converge within {} secs."  # noqa
                        .format(receiver_api, self.dc.get("receiver", "inputCounter"),  # noqa
                                sender_api, self.dc.get("sender", "outputCounter"), self._max_timeout))  # noqa

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, f"Problem killing sender device: '{msg}'.")

        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, f"Problem killing receiver device: '{msg}'.")

    def _test_1to1_drop(self, sender_api, sender_freq,
                        receiver_api, processing_time):
        """
        Test single sender with given frequency (Hz) and single receiver with
        given processing time (ms) where receiver is configured to make sender
        drop.
        """
        print(f"1to1 drop: {sender_api} ({sender_freq} Hz) ==> "
              f"{receiver_api} ({processing_time} ms processing).")
        sender_cfg = Hash("outputFrequency", sender_freq)
        self.start_device(sender_api, 1, "sender", sender_cfg)

        receiver_cfg = Hash("input.connectedOutputChannels", ["sender:output"],
                            "input.onSlowness", "drop",
                            "processingTime", processing_time)
        self.start_device(receiver_api, 1, "receiver", receiver_cfg)

        # wait until receiver is connected
        res = self.waitUntilEqual("receiver", "input.missingConnections", [],
                                  self._max_timeout)
        self.assertTrue(res,
                        "Receiver didn't connect within {self._max_timeout} s")
        # (Only checking "input.missingConnections" not 100% reliable, see
        #  InputChannel::onConnect.)
        assert self.waitUntilOutputKnowsInput("sender", "output",
                                              "receiver:input",
                                              self._max_timeout)

        start_time = time()

        self.dc.execute("sender", "startWritingOutput")

        sleep(self._test_duration)

        self.dc.execute("sender", "stopWritingOutput")

        stop_time = time()

        elapsed_time = stop_time - start_time

        self.assertTrue(
            self.waitUntilEqual(
                "sender", "state",
                State.NORMAL, self._max_timeout),
            "'{}' Sender didn't reach NORMAL state "
            "within {} secs. after stop."
            .format(sender_api, self._max_timeout))
        # Note: Since state has higher priority, its update
        # might have overtaken the last
        #       'outputCounter' update, so out_count may
        # rarely appear too small by one?
        out_count = self.dc.get("sender", "outputCounter")

        # Test that duration and frequency match by +/-25%:
        min_expected = 0.75 * sender_freq * elapsed_time
        max_expected = 1.25 * sender_freq * elapsed_time
        self.assertTrue(min_expected < out_count < max_expected,
                        "# of output data items, {}, is not in the expected interval ({:.2f}, {:.2f})."  # noqa
                        .format(out_count, min_expected, max_expected))

        # Note: We cannot be sure whether pipeline is still active
        inputCounter = self.dc.get("receiver", "inputCounter")

        sender_delay_ms = 1.0/sender_freq * 1000
        if (sender_delay_ms - processing_time > self._drop_noDrop_margin):
            # Receiver is faster than sender by a margin and sender started
            # after receiver claimed to be connected.
            # No drop is expected, but hickups are not excluded, so 5% margin:
            is_in_range = 0.95 * out_count < inputCounter <= out_count
            # Safety loop to work around the two problems noted above
            counter = 0
            while not is_in_range and counter < 50:
                sleep(0.1)
                inputCounter = self.dc.get("receiver", "inputCounter")
                out_count = self.dc.get("sender", "outputCounter")
                is_in_range = 0.95 * out_count < inputCounter <= out_count
                counter += 1

            self.assertTrue(is_in_range,
                            "# of input data items, {}, is not in the expected interval ({:.2f}, {:.2f}]."  # noqa
                            .format(inputCounter, 0.95 * out_count, out_count))
        else:
            # Receiver is not much faster (or is slower) than the sender.
            # Some drop is expected.
            # Assert that at least some data has arrived.
            self.assertGreater(
                inputCounter, 0,
                "At least one data item should have been received.")

        ok, msg = self.dc.killDevice("sender", self._max_timeout)
        self.assertTrue(ok, f"Problem killing sender device: '{msg}'.")

        ok, msg = self.dc.killDevice("receiver", self._max_timeout)
        self.assertTrue(ok, f"Problem killing receiver device: '{msg}'.")

    def start_server_num(self, api, server_num, **kwargs):
        """Start server of given api and number"""

        klasses = ["PropertyTest"]
        server_id = self.serverId(api, server_num)
        self.start_server(api, server_id, klasses, **kwargs)

    def start_device(self, api, server_num, dev_id, cfg):
        """
        Start device with id and config on server defined by api and number
        """
        klass = "PropertyTest"
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
        cfg.set("deviceId", dev_id)

        self.dc.instantiateNoWait(self.serverId(api, server_num),
                                  klass, cfg)

    def serverId(self, api, num):
        """Server id of server with given api and number"""
        return f"{api}Server/{num}"

    def waitUntilEqual(self, devId, propertyName, whatItShouldBe, timeout):
        """
        Wait until property 'propertyName' of device 'deviceId' equals
        'whatItShouldBe'.
        Try up to 'timeOut' seconds and wait 0.1 seconds between each try.
        """
        start = datetime.now()
        while (datetime.now() - start).seconds < timeout:
            res = None
            try:
                res = self.dc.get(devId, propertyName)
            except RuntimeError as re:
                print(f"Problem retrieving property value: {re}")
            if res == whatItShouldBe:
                return True
            else:
                sleep(.1)

        # Maybe we failed due to DeviceClient caching bug? Try direct call!
        # TODO: Remove once the DeviceClient bug is solved.
        caller = SignalSlotable("helperWaitUntilEqual")
        caller.start()

        requestor = caller.request(devId, "slotGetConfiguration")
        dev_props, _ = requestor.waitForReply(self._slot_timeout_ms)
        if (dev_props.get(propertyName) == whatItShouldBe
            or (isinstance(whatItShouldBe, State)
                and dev_props.get(propertyName) == whatItShouldBe.name)):
            print("waitUntilEqual: Suffered from DeviceClient caching bug for",
                  f"{devId}.{propertyName}")
            return True
        else:
            print("waitUntilEqual: DeviceClient not responsible for",
                  f"{devId}.{propertyName} being wrong:",
                  f"{dev_props.get(propertyName)} != {whatItShouldBe}")
            return False

    def waitUntilEqual2(self, devId, propertyName, devId2, propertyName2,
                        timeout):
        """
        Wait until property 'propertyName' of device 'deviceId' equals
        property 'propertyName2' of device 'deviceId2'.
        Try up to 'timeOut' seconds and wait 0.25 seconds between each try.
        """
        start = datetime.now()
        while (datetime.now() - start).seconds < timeout:
            res = None
            try:
                res = self.dc.get(devId, propertyName)
                res2 = self.dc.get(devId2, propertyName2)
            except RuntimeError as re:
                print(f"Problem retrieving property value: {re}")
            if res == res2:
                return True
            else:
                sleep(.25)

        # Maybe we failed due to DeviceClient caching bug? Try direct calls!
        # TODO: Remove once the DeviceClient bug is solved.
        caller = SignalSlotable("helperWaitUntilEqual2")
        caller.start()

        requestor = caller.request(devId, "slotGetConfiguration")
        dev_props, _ = requestor.waitForReply(self._slot_timeout_ms)
        res = dev_props.get(propertyName)

        requestor = caller.request(devId2, "slotGetConfiguration")
        dev_props2, _ = requestor.waitForReply(self._slot_timeout_ms)
        res2 = dev_props2.get(propertyName2)

        if res == res2:
            print(
                "waitUntilEqual2: Suffered from DeviceClient caching bug for",
                f"{devId}.{propertyName} and {devId2}.{propertyName2}")
            return True
        else:
            print("waitUntilEqual2: DeviceClient not responsible for",
                  f"{devId}.{propertyName} != {devId2}.{propertyName2}:",
                  f"{res} vs {res2}")
            return False

    def waitUntilOutputKnowsInput(self, outputDevId, output, inputId, timeout):
        """
        Wait until an output channel lists an input in its connections

        :param outputDevId deviceId of device with output channel
        :param output key of output channel in schema of outputDevId
        :param inputId id of expected input channel, e.g. "receiver:input"
        :return boolean whether successfully waited (else timeout)
        """

        def condition():
            connections = self.dc.get(outputDevId, output + ".connections")
            for connection in connections:
                if connection["remoteId"] == inputId:
                    return True
            return False

        return self.waitUntilTrue(condition, timeout)
