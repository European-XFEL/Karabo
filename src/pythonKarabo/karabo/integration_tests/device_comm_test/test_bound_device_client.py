from time import sleep

from karabo.bound import Hash, InputChannel
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

        # data handler
        @call_counter
        def on_data(data, metadata):
            self.msg_counter += 1

            assert isinstance(data, Hash)
            assert metadata['source'] == f'{self.data_device}:output'
            assert data['node.int32'] == self.msg_counter

        # input handler
        @call_counter
        def on_input(channel):
            self.msg_counter += 1

            assert isinstance(channel, InputChannel)
            assert channel.size() == 1
            metadata = channel.getMetaData()
            assert len(metadata) == 1
            assert metadata[0]['source'] == f'{self.data_device}:output'
            data = channel.read(0)

            assert data['node.int32'] == self.msg_counter

        # eos handler
        @call_counter
        def on_eos(channel):
            assert isinstance(channel, InputChannel)

        assert self.dc.registerChannelMonitor(channel, dataHandler=on_data,
                                              inputChannelCfg=config)
        for i in range(3):
            self.dc.execute(self.data_device, 'writeOutput')

        for i in range(100):
            sleep(0.01)
            if on_data._calls >= 3:
                break

        assert on_data._calls == 3
        assert self.dc.unregisterChannelMonitor(channel)

        assert self.dc.registerChannelMonitor(
            channel, inputChannelCfg=config, eosHandler=on_eos,
            inputHandler=on_input)
        assert not self.dc.registerChannelMonitor(
            channel, inputChannelCfg=config, inputHandler=on_input
        )  # already monitoring this output channel

        for i in range(5):
            self.dc.execute(self.data_device, 'writeOutput')

        for i in range(100):
            sleep(0.01)
            if on_input._calls >= 5:
                break
        assert on_input._calls == 5

        self.dc.execute(self.data_device, 'eosOutput')

        for i in range(100):
            sleep(0.01)
            if on_eos._calls > 0:
                break
        assert on_eos._calls == 1

        assert self.dc.unregisterChannelMonitor(channel)
        assert not self.dc.unregisterChannelMonitor(channel)
