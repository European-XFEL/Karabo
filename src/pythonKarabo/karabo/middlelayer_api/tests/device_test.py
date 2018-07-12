from asyncio import coroutine, sleep
from contextlib import contextmanager
from unittest import main

import numpy as np

from karabo.common.states import State
from karabo.middlelayer import (
    AccessMode, getDevice, Int32, waitUntil, waitWhile)
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import getSchema
from karabo.middlelayer_api.hash import Float, Hash, Slot, VectorHash
from karabo.middlelayer_api.pipeline import InputChannel, OutputChannel
from karabo.middlelayer_api.schema import Configurable, Node

from .eventloop import async_tst, DeviceTest, sync_tst


class RowSchema(Configurable):
    x = Float(defaultValue=1.0)
    y = Float(defaultValue=1.0)


class Data(Configurable):
    floatProperty = Float(displayedName="Float")


class MyNode(Configurable):
    output = OutputChannel()


class MyDevice(Device):
    __version__ = "1.2"

    counter = Int32(
        defaultValue=0,
    )

    # an output channel without schema
    output = OutputChannel()
    dataOutput = OutputChannel(Data)

    deep = Node(MyNode)

    @InputChannel()
    def input(self, data, meta):
        pass

    table = VectorHash(
        rows=RowSchema,
        displayedName="Table",
        defaultValue=[Hash("x", 2.0, "y", 5.6)],
        accessMode=AccessMode.RECONFIGURABLE)

    @Slot(displayedName="Increase", allowedStates=[State.ON])
    @coroutine
    def increaseCounter(self):
        self.state = State.ACQUIRING
        self.counter += 1
        yield from sleep(0.2)
        self.state = State.ON

    @Slot(displayedName="start")
    @coroutine
    def start(self):
        return 5

    @coroutine
    def onInitialization(self):
        self.state = State.ON


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.myDevice = MyDevice(dict(_deviceId_='MyDevice'))
        with cls.deviceManager(lead=cls.myDevice):
            yield

    @sync_tst
    def test_device_version(self):
        expected = "1.2"
        self.assertEqual(self.myDevice.classVersion, expected)

    @sync_tst
    def test_output_names(self):
        names = self.myDevice.slotGetOutputChannelNames()
        expected = ['dataOutput', 'deep.output', 'output']
        self.assertEqual(names, expected)

    @sync_tst
    def test_displayType_state(self):
        self.assertEqual(self.myDevice.state.descriptor.displayType, 'State')

    @sync_tst
    def test_displayType_output(self):
        self.assertEqual(self.myDevice.output.displayType, 'OutputChannel')
        self.assertEqual(self.myDevice.dataOutput.displayType, 'OutputChannel')

    @sync_tst
    def test_displayType_input(self):
        self.assertEqual(self.myDevice.input.displayType, 'InputChannel')

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

    @async_tst
    def test_lastCommand(self):
        self.assertEqual(self.myDevice.lastCommand, "")
        with (yield from getDevice("MyDevice")) as d:
            yield from d.start()
        self.assertEqual(self.myDevice.lastCommand, "start")
        yield from getSchema("MyDevice")
        self.assertEqual(self.myDevice.lastCommand, "start")

    @async_tst
    def test_two_calls_concurrent(self):
        self.assertEqual(self.myDevice.counter, 0)
        with (yield from getDevice("MyDevice")) as d:
            yield from d.increaseCounter()
            self.assertEqual(self.myDevice.counter, 1)
            # Concurrent slot calls, one will return due to state block
            self.myDevice._ss.emit("call", {"MyDevice": ["increaseCounter",
                                                         "increaseCounter"]})
            yield from waitUntil(lambda: d.state != State.ON)
            yield from waitWhile(lambda: d.state == State.ACQUIRING)
            self.assertEqual(self.myDevice.counter, 2)

    @async_tst
    def test_clear_table_external(self):
        with (yield from getDevice("MyDevice")) as d:
            dtype = d.table.descriptor.dtype
            current_value = np.array((2.0, 5.6),
                                     dtype=dtype)
            self.assertEqual(d.table.value, current_value)
            # clear the value on the device side. The values are popped.
            d.table.clear()
            empty_table = np.array([], dtype=dtype)
            success = np.array_equal(d.table.value, empty_table)
            self.assertTrue(success)

    @async_tst
    def test_output_close(self):
        self.assertIsNotNone(self.myDevice.output.server.sockets)
        yield from self.myDevice.output.close()
        self.assertIsNone(self.myDevice.output.server.sockets)


if __name__ == '__main__':
    main()
