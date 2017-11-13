from contextlib import contextmanager
from unittest import main

from karabo.middlelayer import Hash
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.pipeline import OutputChannel
from karabo.middlelayer_api.schema import Configurable, Node

from .eventloop import DeviceTest, async_tst, sync_tst


class MyNode(Configurable):
    output = OutputChannel()


class MyDevice(Device):
    # an output channel without schema
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

    @async_tst
    def test_send_raw(self):
        hsh = Hash("Itchy", 10)
        self.assertIsNone(self.myDevice.output.schema)
        yield from self.myDevice.output.writeRawData(hsh)

        # provoke attribute error because we don't have a schema
        with self.assertRaises(AttributeError):
            yield from self.myDevice.output.writeData(hsh)

    @sync_tst
    def test_send_raw_no_wait(self):
        hsh = Hash("Scratchy", 20)
        self.assertIsNone(self.myDevice.output.schema)
        self.myDevice.output.writeRawDataNoWait(hsh)

        # provoke attribute error because we don't have a schema
        with self.assertRaises(AttributeError):
            self.myDevice.output.writeDataNoWait(hsh)


if __name__ == '__main__':
    main()
