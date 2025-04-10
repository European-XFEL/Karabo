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
from threading import Lock
from time import sleep

from karabo.bound import ConnectionStatus, Hash, InputChannel
from karabo.bound.testing import BoundDeviceTestCase

# Sleeping loops of up to 5 seconds to let things happen on busy CI
# (failed once with 2 seconds)
n_loops = 500
sleep_in_loop = 0.01


def call_counter(func):
    def inner(*args, **kwargs):
        # Protect _calls and 'func' evaluation if called from different C++
        # threads by lock. Access of variables touched inside 'func' need the
        # same protection
        with inner._lock:
            inner._calls += 1
            return func(*args, **kwargs)
    inner._lock = Lock()
    inner._calls = 0
    return inner


class TestDeviceClientComm(BoundDeviceTestCase):
    def setUp(self):
        super().setUp()

        self.data_server = 'BoundDeviceServer0'
        self.data_device = 'propTestDevice0'
        self.start_server("bound", self.data_server, ["PropertyTest"])
        self.instantiate_device(self.data_server, deviceId=self.data_device)

    def tearDown(self):
        super().tearDown()

    def instantiate_device(self, serverId, classId='PropertyTest',
                           deviceId='propTestDevice', configuration=None,
                           timeout=30):
        config = Hash('classId', classId, 'deviceId', deviceId,
                      'configuration', configuration or Hash())
        return self.dc.instantiate(serverId, config, timeout)

    def test_mdl_schema(self):
        # We test that the Schema functions work also for schema from MDL

        server_mdl = 'MdlDeviceServer0'
        devId = "propTestDeviceMdl"
        self.start_server("mdl", server_mdl, ["PropertyTest"])
        self.instantiate_device(server_mdl, classId="PropertyTest",
                                deviceId=devId)

        schema = self.dc.getDeviceSchema(devId)

        # state and alarmCondition have "leafType" attribute,
        # normal properties do not.
        self.assertTrue(schema.isProperty("state"))
        self.assertTrue(schema.isProperty("alarmCondition"))
        self.assertTrue(schema.isProperty("boolProperty"))
        self.assertFalse(schema.isProperty("slotClearLock"))

        self.assertFalse(schema.isCommand("state"))
        self.assertFalse(schema.isCommand("alarmCondition"))
        self.assertFalse(schema.isCommand("boolProperty"))
        self.assertTrue(schema.isCommand("slotClearLock"))

    def test_channel_monitor(self):
        channel = f'{self.data_device}:output'
        config = Hash('onSlowness', 'wait', 'dataDistribution', 'copy')

        # Note: We know from C++ code that data, input, and eos handlers are
        #       never called in parallel. So it does not harm if access to
        #       'called_with' and 'msg_counter' is only protected by any of the
        #       handlers' locks
        called_with = {}
        msg_counter = 0

        # data handler
        @call_counter
        def on_data(data, metadata):
            nonlocal msg_counter
            msg_counter += 1

            # Any assert directly here leads, in case of failure, to print out
            # about exceptions, but test claims to be OK!
            called_with["dataInstanceHash"] = isinstance(data, Hash)
            called_with["dataSource"] = metadata['source']
            called_with["dataInt32"] = data['node.int32']

        # input handler
        @call_counter
        def on_input(channel):
            nonlocal msg_counter
            msg_counter += 1

            called_with["inputIsChannel"] = isinstance(channel, InputChannel)
            called_with["inputSize"] = channel.size()
            metadata = channel.getMetaData()
            called_with["inputLenMetadata"] = len(metadata)
            called_with["inputSource"] = metadata[0]['source']
            data = channel.read(0)
            called_with["inputInt32"] = data['node.int32']

        # eos handler
        @call_counter
        def on_eos(channel):
            called_with["eosIsChannel"] = isinstance(channel, InputChannel)

        status_list = []

        # status tracker
        @call_counter
        def status_tracker(status):
            status_list.append(status)

        assert self.dc.registerChannelMonitor(channel, dataHandler=on_data,
                                              inputChannelCfg=config,
                                              statusTracker=status_tracker)

        for i in range(n_loops):
            with status_tracker._lock:
                if status_tracker._calls >= 2:
                    break
            sleep(sleep_in_loop)

        with status_tracker._lock:
            self.assertEqual(2, status_tracker._calls)
            self.assertEqual(ConnectionStatus.CONNECTING, status_list[0])
            self.assertEqual(ConnectionStatus.CONNECTED, status_list[1])

        for i in range(3):
            self.dc.execute(self.data_device, 'writeOutput')

        for i in range(n_loops):
            sleep(sleep_in_loop)
            with on_data._lock:
                if on_data._calls >= 3:
                    break

        with on_data._lock:
            assert on_data._calls == 3

            self.assertTrue(called_with["dataInstanceHash"])
            self.assertEqual(f'{self.data_device}:output',
                             called_with["dataSource"])
            self.assertEqual(msg_counter, called_with["dataInt32"])

        assert self.dc.unregisterChannelMonitor(channel)
        # unregistration triggers a DISCONNECTED status call
        for i in range(n_loops):
            with status_tracker._lock:
                if status_tracker._calls >= 3:
                    break
            sleep(sleep_in_loop)

        with status_tracker._lock:
            self.assertEqual(3, status_tracker._calls)
            self.assertEqual(ConnectionStatus.DISCONNECTED, status_list[2])

        # Now test with input handler
        with on_input._lock:
            called_with.clear()
        assert self.dc.registerChannelMonitor(
            channel, inputChannelCfg=config, eosHandler=on_eos,
            inputHandler=on_input, statusTracker=status_tracker)
        assert not self.dc.registerChannelMonitor(
            channel, inputChannelCfg=config, inputHandler=on_input
        )  # already monitoring this output channel

        # Now two more calls to connection status tracker
        for i in range(n_loops):
            with status_tracker._lock:
                if status_tracker._calls >= 5:
                    break
            sleep(sleep_in_loop)

        with status_tracker._lock:
            self.assertEqual(5, status_tracker._calls,
                             f"wrong calls: {status_list}")
            self.assertEqual(ConnectionStatus.CONNECTING, status_list[-2])
            self.assertEqual(ConnectionStatus.CONNECTED, status_list[-1])

        for i in range(5):
            self.dc.execute(self.data_device, 'writeOutput')

        for i in range(n_loops):
            sleep(sleep_in_loop)
            with on_input._lock:
                if on_input._calls >= 5:
                    break
        with on_input._lock:
            assert on_input._calls == 5

            self.assertTrue(called_with["inputIsChannel"])
            self.assertEqual(called_with["inputSize"], 1)
            self.assertEqual(called_with["inputLenMetadata"], 1)
            self.assertEqual(f'{self.data_device}:output',
                             called_with["inputSource"])
            self.assertEqual(msg_counter, called_with["inputInt32"])

        self.dc.execute(self.data_device, 'eosOutput')

        for i in range(n_loops):
            sleep(sleep_in_loop)
            with on_eos._lock:
                if on_eos._calls > 0:
                    break
        with on_eos._lock:
            assert on_eos._calls == 1

            self.assertTrue(called_with["eosIsChannel"])

        assert self.dc.unregisterChannelMonitor(channel)
        assert not self.dc.unregisterChannelMonitor(channel)
        for i in range(n_loops):
            with status_tracker._lock:
                if status_tracker._calls >= 6:
                    break
            sleep(sleep_in_loop)

        with status_tracker._lock:
            self.assertEqual(6, status_tracker._calls)
            self.assertEqual(ConnectionStatus.DISCONNECTED, status_list[5])

        # Now test simultaneous data and input handlers:
        with on_eos._lock:
            called_with.clear()
        ok = self.dc.registerChannelMonitor(channel,
                                            dataHandler=on_data,
                                            inputHandler=on_input,
                                            statusTracker=status_tracker)
        self.assertTrue(ok)

        # Wait until connected
        # (i.e. two more calls to statusTracker: CONNECTING, CONNECTED)
        for i in range(n_loops):
            with status_tracker._lock:
                if status_tracker._calls >= 8:
                    break
            sleep(sleep_in_loop)

        with status_tracker._lock:
            self.assertEqual(8, status_tracker._calls,
                             f"wrong calls: {status_list}")
            self.assertEqual(ConnectionStatus.CONNECTING, status_list[-2])
            self.assertEqual(ConnectionStatus.CONNECTED, status_list[-1])

        self.dc.execute(self.data_device, 'writeOutput')

        # Wait until both handlers called one more time
        for i in range(n_loops):
            sleep(sleep_in_loop)
            with on_input._lock:
                if on_input._calls >= 6:
                    with on_data._lock:
                        if on_data._calls >= 4:
                            break

        # Check overal handler calls
        with on_input._lock:
            self.assertEqual(6, on_input._calls)
        with on_data._lock:
            self.assertEqual(4, on_data._calls)
            self.assertEqual(10, msg_counter)
            # Data of both handlers is the same:
            self.assertEqual(9, called_with["dataInt32"])
            self.assertEqual(9, called_with["inputInt32"])

        self.assertTrue(self.dc.unregisterChannelMonitor(channel))
        for i in range(n_loops):
            with status_tracker._lock:
                if status_tracker._calls >= 9:
                    break
            sleep(sleep_in_loop)

        with status_tracker._lock:
            self.assertEqual(9, status_tracker._calls)
            self.assertEqual(ConnectionStatus.DISCONNECTED, status_list[8])
