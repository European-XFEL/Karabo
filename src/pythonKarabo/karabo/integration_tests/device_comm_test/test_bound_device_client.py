from time import sleep

from karabo.bound import ConnectionStatus, Hash, InputChannel
from karabo.integration_tests.utils import BoundDeviceTestCase

def call_counter(func):
    def inner(*args, **kwargs):
        inner._calls += 1
        return func(*args, **kwargs)
    inner._calls = 0
    return inner


class TestDeviceClientComm(BoundDeviceTestCase):
    def setUp(self):
        super(TestDeviceClientComm, self).setUp()

        self.data_server = 'BoundDeviceServer0'
        self.data_device = 'propTestDevice0'
        self.start_server("bound", self.data_server, ["PropertyTest"])
        self.instantiate_device(self.data_server, deviceId=self.data_device)
        self.msg_counter = 0

    def tearDown(self):
        super(TestDeviceClientComm, self).tearDown()

    def instantiate_device(self, serverId, classId='PropertyTest',
                           deviceId='propTestDevice', configuration=None,
                           timeout=30):
        config = Hash('classId', classId, 'deviceId', deviceId,
                      'configuration', configuration or Hash())
        return self.dc.instantiate(serverId, config, timeout)

    def test_channel_monitor(self):
        channel = f'{self.data_device}:output'
        config = Hash('onSlowness', 'wait', 'dataDistribution', 'copy')

        called_with = {}
        # data handler
        @call_counter
        def on_data(data, metadata):
            self.msg_counter += 1

            # Any assert directly here leads, in case of failure, to print out
            # about exceptions, but test claims to be OK!
            called_with["dataInstanceHash"] = isinstance(data, Hash)
            called_with["dataSource"] = metadata['source']
            called_with["dataInt32"] = data['node.int32']

        # input handler
        @call_counter
        def on_input(channel):
            self.msg_counter += 1

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

        # status tracker
        status_list = []
        @call_counter
        def status_tracker(status):
            status_list.append(status)

        assert self.dc.registerChannelMonitor(channel, dataHandler=on_data,
                                              inputChannelCfg=config,
                                              statusTracker=status_tracker)

        for i in range(100):
            if status_tracker._calls >= 2:
                break
            sleep(0.01)

        self.assertEqual(2, status_tracker._calls)
        self.assertEqual(ConnectionStatus.CONNECTING, status_list[0])
        self.assertEqual(ConnectionStatus.CONNECTED, status_list[1])

        for i in range(3):
            self.dc.execute(self.data_device, 'writeOutput')

        for i in range(100):
            sleep(0.01)
            if on_data._calls >= 3:
                break

        assert on_data._calls == 3

        self.assertTrue(called_with["dataInstanceHash"])
        self.assertEqual(f'{self.data_device}:output',
                         called_with["dataSource"])
        self.assertEqual(self.msg_counter, called_with["dataInt32"])

        assert self.dc.unregisterChannelMonitor(channel)

        # Now test with input handler
        called_with.clear()
        assert self.dc.registerChannelMonitor(
            channel, inputChannelCfg=config, eosHandler=on_eos,
            inputHandler=on_input, statusTracker=status_tracker)
        assert not self.dc.registerChannelMonitor(
            channel, inputChannelCfg=config, inputHandler=on_input
        )  # already monitoring this output channel

        # Now two more calls to connection status tracker
        for i in range(100):
            if status_tracker._calls >= 4:
                break
            sleep(0.01)

        self.assertEqual(4, status_tracker._calls)
        # No DISCONNECTING since channel killed when unregisterChannelMonitor
        self.assertEqual(ConnectionStatus.CONNECTING, status_list[2])
        self.assertEqual(ConnectionStatus.CONNECTED, status_list[3])

        for i in range(5):
            self.dc.execute(self.data_device, 'writeOutput')

        for i in range(100):
            sleep(0.01)
            if on_input._calls >= 5:
                break
        assert on_input._calls == 5

        self.assertTrue(called_with["inputIsChannel"])
        self.assertEqual(called_with["inputSize"], 1)
        self.assertEqual(called_with["inputLenMetadata"], 1)
        self.assertEqual(f'{self.data_device}:output',
                         called_with["inputSource"])
        self.assertEqual(self.msg_counter, called_with["inputInt32"])

        self.dc.execute(self.data_device, 'eosOutput')

        for i in range(100):
            sleep(0.01)
            if on_eos._calls > 0:
                break
        assert on_eos._calls == 1

        self.assertTrue(called_with["eosIsChannel"])

        assert self.dc.unregisterChannelMonitor(channel)
        assert not self.dc.unregisterChannelMonitor(channel)

        # Now test simultaneous data and input handlers:
        called_with.clear()
        ok = self.dc.registerChannelMonitor(channel,
                                            dataHandler=on_data,
                                            inputHandler=on_input,
                                            statusTracker=status_tracker)
        self.assertTrue(ok)

        # Wait until connected (i.e. two more calls to statusTracker)
        for i in range(100):
            if status_tracker._calls >= 6:
                break
            sleep(0.01)

        self.dc.execute(self.data_device, 'writeOutput')

        # Wait until both handlers called one more time
        for i in range(100):
            sleep(0.01)
            if on_input._calls >= 6 and on_data._calls >= 4:
                break

        # Check overal handler calls
        self.assertEqual(6, on_input._calls)
        self.assertEqual(4, on_data._calls)
        self.assertEqual(10, self.msg_counter)
        # Data of both handlers is the same:
        self.assertEqual(9, called_with["dataInt32"])
        self.assertEqual(9, called_with["inputInt32"])

        self.assertTrue(self.dc.unregisterChannelMonitor(channel))
