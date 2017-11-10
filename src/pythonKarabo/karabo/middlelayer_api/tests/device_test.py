from contextlib import contextmanager
from unittest import main

from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.pipeline import OutputChannel
from karabo.middlelayer_api.schema import Configurable, Node

from .eventloop import DeviceTest, sync_tst


class MyNode(Configurable):
    output = OutputChannel()


class MyDevice(Device):
    output = OutputChannel()
    deep = Node(MyNode)


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.myDevice = MyDevice(dict(_deviceId_='MyDevice'))
        with cls.deviceManager(lead=cls.myDevice):
            yield

    @sync_tst
    def test_output_names(self):
        names = self.myDevice.slotGetOutputChannelNames()
        expected = ['deep.output', 'output']
        self.assertEqual(names, expected)

    @sync_tst
    def test_displayType_state(self):
        self.assertEqual(self.myDevice.state.descriptor.displayType, 'State')


if __name__ == '__main__':
    main()
