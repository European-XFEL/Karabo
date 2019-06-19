from asyncio import sleep
from contextlib import contextmanager
from unittest import main

import numpy as np

from karabo.common.states import State
from karabo.middlelayer import (
    AccessMode, background, getDevice, Int32, waitUntil, waitWhile)
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import call, getSchema
from karabo.native.data.hash import Float, Hash, Slot, VectorHash
from karabo.native.timestamp import Timestamp
from karabo.middlelayer_api.pipeline import InputChannel, OutputChannel
from karabo.middlelayer_api.utils import get_property
from karabo.native.data.schema import Configurable, Node

from .eventloop import async_tst, DeviceTest, sync_tst


class RowSchema(Configurable):
    x = Float(defaultValue=1.0)
    y = Float(defaultValue=1.0)


class Data(Configurable):
    floatProperty = Float(defaultValue=10,
                          displayedName="Float")


class MyNode(Configurable):
    output = OutputChannel()


class MyDevice(Device):
    __version__ = "2.2"

    integer = Int32(
        defaultValue=0,
        minInc=0,
        maxInc=10)

    counter = Int32(
        defaultValue=0,
        minInc=0,
        maxInc=2000)

    # an output channel without schema
    output = OutputChannel()
    dataOutput = OutputChannel(Data)

    deep = Node(MyNode)
    nested = Node(Data)

    @InputChannel()
    async def input(self, data, meta):
        """The input channel with data and meta
        """

    table = VectorHash(
        rows=RowSchema,
        displayedName="Table",
        defaultValue=[Hash("x", 2.0, "y", 5.6),
                      Hash("x", 1.0, "y", 1.6)],
        accessMode=AccessMode.RECONFIGURABLE)

    @Slot(displayedName="Increase", allowedStates=[State.ON])
    async def increaseCounter(self):
        self.state = State.ACQUIRING
        self.counter = self.counter.value + 1
        background(self._state_sleep)

    async def _state_sleep(self):
        await sleep(0.2)
        self.state = State.ON

    @Slot(displayedName="start")
    async def start(self):
        return 5

    async def onInitialization(self):
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
        expected = "2.2"
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

    @sync_tst
    def test_classId_output(self):
        self.assertEqual(self.myDevice.output.classId, 'OutputChannel')
        self.assertEqual(self.myDevice.dataOutput.classId, 'OutputChannel')

    @sync_tst
    def test_classId_input(self):
        self.assertEqual(self.myDevice.input.classId, 'InputChannel')

    @async_tst
    async def test_classId_slot(self):
        s = await getSchema("MyDevice")
        self.assertTrue(s.hash.hasAttribute('start', 'classId'),
                        "Attribute 'classId' is missing from Slot schema.")
        self.assertEqual(s.hash.getAttribute('start', 'classId'), 'Slot')

    @async_tst
    async def test_classId_node(self):
        s = await getSchema("MyDevice")
        self.assertFalse(s.hash.hasAttribute('deep', 'classId'),
                         "Node should not have a 'classId' attribute.")

    @async_tst
    async def test_send_raw(self):
        hsh = Hash("Itchy", 10)
        self.assertIsNone(self.myDevice.output.schema)
        await self.myDevice.output.writeRawData(hsh)

        # provoke attribute error because we don't have a schema
        with self.assertRaises(AttributeError):
            await self.myDevice.output.writeData(hsh)

    @sync_tst
    def test_send_raw_no_wait(self):
        hsh = Hash("Scratchy", 20)
        self.assertIsNone(self.myDevice.output.schema)
        self.myDevice.output.writeRawDataNoWait(hsh)

        # provoke attribute error because we don't have a schema
        with self.assertRaises(AttributeError):
            self.myDevice.output.writeDataNoWait(hsh)

    @async_tst
    async def test_lastCommand(self):
        self.assertEqual(self.myDevice.lastCommand, "")
        with (await getDevice("MyDevice")) as d:
            await d.start()
        self.assertEqual(self.myDevice.lastCommand, "start")
        await getSchema("MyDevice")
        self.assertEqual(self.myDevice.lastCommand, "start")

    @async_tst
    async def test_two_calls_concurrent(self):
        self.assertEqual(self.myDevice.counter, 0)
        with (await getDevice("MyDevice")) as d:
            await d.increaseCounter()
            await waitUntil(lambda: d.state != State.ON)
            await waitWhile(lambda: d.state == State.ACQUIRING)
            self.assertEqual(self.myDevice.counter, 1)
            # Concurrent slot calls, one will return due to state block
            self.myDevice._ss.emit("call", {"MyDevice": ["increaseCounter",
                                                         "increaseCounter"]})
            await waitUntil(lambda: d.state != State.ON)
            await waitWhile(lambda: d.state == State.ACQUIRING)
            self.assertEqual(self.myDevice.counter, 2)

    @async_tst
    async def test_clear_table_external(self):
        with (await getDevice("MyDevice")) as d:
            dtype = d.table.descriptor.dtype
            current_value = np.array((2.0, 5.6), dtype=dtype)
            self.assertEqual(d.table[0].value, current_value)

            # pop the first item and compare
            check_value = d.table.pop(0)
            self.assertEqual(check_value.value, current_value)

            # The former second is now first
            current_value = np.array((1.0, 1.6), dtype=dtype)
            self.assertEqual(d.table[0].value, current_value)
            self.assertEqual(len(d.table.value), 1)

            # clear the value on the device side. The values are popped.
            d.table.clear()
            empty_table = np.array([], dtype=dtype)
            success = np.array_equal(d.table.value, empty_table)
            self.assertTrue(success)

    @async_tst
    async def test_output_close(self):
        self.assertIsNotNone(self.myDevice.output.server.sockets)
        await self.myDevice.output.close()
        self.assertIsNone(self.myDevice.output.server.sockets)

    @sync_tst
    def test_slot_verification(self):
        self.assertEqual(self.myDevice.slotHasSlot("increaseCounter"), True)
        self.assertEqual(self.myDevice.slotHasSlot("output"), False)
        self.assertEqual(self.myDevice.slotHasSlot("doesNotExist"), False)

    @sync_tst
    def test_output_information(self):
        device = self.myDevice
        # Second argument processId is not used in MDL
        success, data = yield from device.slotGetOutputChannelInformation(
            "output", None)
        self.assertEqual(success, True)
        self.assertEqual(data["hostname"], self.myDevice.hostName)
        self.assertEqual(data["connectionType"], "tcp")
        self.assertEqual(data["memoryLocation"], "remote")
        self.assertIsInstance(data["port"], np.uint32)

        success, data = yield from device.slotGetOutputChannelInformation(
            "doesNotExist", None)
        self.assertEqual(success, False)
        self.assertEqual(data, Hash())

    @async_tst
    async def test_applyRunTimeUpdates(self):
        with (await getDevice("MyDevice")) as d:
            self.assertEqual(d.counter.descriptor.minInc, 0)
            self.assertEqual(d.counter.descriptor.maxInc, 2000)

        updates = [Hash('path', "counter",
                        'attribute', "maxInc",
                        'value', 100.0)]
        reply = await self.myDevice.slotUpdateSchemaAttributes(updates)
        self.assertTrue(reply['success'])

        with (await getDevice("MyDevice")) as d:
            self.assertEqual(d.counter.descriptor.minInc, 0)
            self.assertEqual(d.counter.descriptor.maxInc, 100)

        updates = [Hash('path', "nocounter",
                        'attribute', "maxExc",
                        'value', 100.0)]
        reply = await self.myDevice.slotUpdateSchemaAttributes(updates)
        self.assertFalse(reply['success'])

    @async_tst
    async def test_get_property(self):
        prop = get_property(self.myDevice, "integer")
        self.assertEqual(prop, 0)
        prop = get_property(self.myDevice, "nested.floatProperty")
        self.assertEqual(prop, 10)
        with self.assertRaises(AttributeError):
            get_property(self.myDevice, "nested.notthere")
        with self.assertRaises(AttributeError):
            get_property(self.myDevice, "notthere")

    @async_tst
    async def test_slot_time(self):
        h = await call("MyDevice", "slotGetTime")
        self.assertIsNotNone(h)
        self.assertTrue(h["time"])
        self.assertGreater(h.getAttributes("time")["sec"], 0)
        self.assertIsNotNone(h.getAttributes("time")["frac"], 0)
        self.assertEqual(h.getAttributes("time")["tid"], 0)
        timestamp_first = Timestamp.fromHashAttributes(h["time", ...])
        h = await call("MyDevice", "slotGetTime")
        timestamp_second = Timestamp.fromHashAttributes(h["time", ...])
        self.assertGreater(timestamp_second, timestamp_first)


if __name__ == '__main__':
    main()
