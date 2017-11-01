from contextlib import contextmanager
from unittest import main

from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.pipeline import OutputChannel
from karabo.middlelayer_api.schema import Configurable, Node

from .eventloop import DeviceTest, sync_tst


class MyNode(Configurable):
    output = OutputChannel()


class Remote(Device):
    output = OutputChannel()
    deep = Node(MyNode)


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.remote = Remote(dict(_deviceId_="remote"))
        with cls.deviceManager(lead=cls.remote):
            yield

    @sync_tst
    def test_output_names(self):
        names = self.remote.slotGetOutputChannelNames()
        expected = ["deep.output", "output"]
        self.assertEqual(names, expected)


if __name__ == "__main__":
    main()
