from asyncio import TimeoutError, get_event_loop, sleep
from contextlib import contextmanager
from unittest import main

import numpy as np

from karabo import __version__ as karaboVersion
from karabo.common.states import State
from karabo.middlelayer import (
    AccessMode, KaraboError, background, getDevice, setWait, sleep as mdlsleep,
    waitUntil, waitWhile)
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    call, get_instance, getProperties, getSchema)
from karabo.middlelayer_api.pipeline import InputChannel, OutputChannel
from karabo.middlelayer_api.tests.eventloop import (
    DeviceTest, async_tst, sync_tst)
from karabo.middlelayer_api.utils import AsyncTimer, get_property, set_property
from karabo.native import (
    Bool, Configurable, Float, Hash, Int32, Node, Slot, Timestamp, VectorHash)


class RowSchema(Configurable):
    x = Float(defaultValue=1.0)
    y = Float(defaultValue=1.0)


class ChannelData(Configurable):
    """The properties for the pipeline channels"""
    floatProperty = Float(defaultValue=10.0,
                          displayedName="Float")


class PropertyData(Configurable):
    floatProperty = Float(defaultValue=10.0,
                          displayedName="Float",
                          allowedStates=[State.ON])
    intProperty = Int32(defaultValue=7,
                        displayedName="Integer",
                        allowedStates=[State.ERROR])


class ChannelNode(Configurable):
    """A node with an output channel"""
    output = OutputChannel()


class NodeWithSlot(Configurable):
    """A slot in a node"""
    @Slot()
    async def pressMe(self):
        return 6


class MyDevice(Device):
    __version__ = "1.2.3"

    preInitError = Bool(
        defaultValue=False,
        description="Decide if this device's preInitialization should raise")

    initError = Bool(
        defaultValue=False,
        description="Decide if this device should throw an error in "
                    "onInitialization")

    isDown = Bool(
        defaultValue=False,
        description="Indicator if this device went down. This is set"
                    "on slotKillDevice")

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
    # output channel with schema
    dataOutput = OutputChannel(ChannelData)
    # output channel in a node
    nodeOutput = Node(ChannelNode)

    noded = Node(PropertyData)

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

    async def set_state(self, state):
        self.state = state

    @Slot(displayedName="start")
    async def start(self):
        return 5

    async def preInitialization(self):
        if self.preInitError:
            raise RuntimeError("I am going down")

    async def onInitialization(self):
        self.state = State.ON
        if self.initError:
            raise RuntimeError("Status must not be empty")

    async def slotKillDevice(self):
        self.isDown = True
        return await super().slotKillDevice()

    async def setState(self, state):
        self.state = state

    nodeWithSlot = Node(NodeWithSlot)


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.myDevice = MyDevice(dict(_deviceId_='MyDevice'))
        with cls.deviceManager(lead=cls.myDevice):
            yield

    @sync_tst
    def test_device_version(self):
        expected = "karabo-1.2.3"
        self.assertEqual(self.myDevice.classVersion, expected)
        self.assertEqual(self.myDevice.karaboVersion, karaboVersion)
        # Testing the internals of the Device.__init__ / InjectMixin.__new__
        # interplay. But otherwise we would not test that __module_orig__ is
        # really the original module, also for non-framework classes (because
        # we use only the first part and that is 'karabo' for devices from
        # anywhere in the frameowrk, including test classes.
        self.assertEqual(self.myDevice.__module_orig__, self.__module__)

    @sync_tst
    def test_output_names(self):
        names = self.myDevice.slotGetOutputChannelNames()
        expected = ['dataOutput', 'nodeOutput.output', 'output']
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
    async def test_state_dependent_schema(self):
        schema_before = await getSchema("MyDevice", onlyCurrentState=False)
        # The notation `before` refers to the device in state ON.
        with (await getDevice("MyDevice")) as d:
            # Set a transient state
            await self.myDevice.set_state(State.ACQUIRING)
            proxy_before_schema = await getSchema(d, onlyCurrentState=False)
            self.assertEqual(schema_before.hash.paths(),
                             proxy_before_schema.hash.paths())
            schema_after = await getSchema("MyDevice", onlyCurrentState=True)
            proxy_after_schema = await getSchema(d, onlyCurrentState=True)
            # Back to State.ON
            await self.myDevice.set_state(State.ON)
            self.assertNotEqual(proxy_before_schema.hash.paths(),
                                proxy_after_schema.hash.paths())
            self.assertNotEqual(schema_before.hash.paths(),
                                schema_after.hash.paths())
            # Check slot in schemas before (ON) and after (ACQUIRING)
            self.assertIn("increaseCounter", schema_before.hash)
            self.assertNotIn("increaseCounter", schema_after.hash)
            self.assertIn("increaseCounter", proxy_before_schema.hash)
            self.assertNotIn("increaseCounter", proxy_after_schema.hash)

    @async_tst
    async def test_classId_slot(self):
        s = await getSchema("MyDevice")
        self.assertTrue(s.hash.hasAttribute('start', 'classId'),
                        "Attribute 'classId' is missing from Slot schema.")
        self.assertEqual(s.hash.getAttribute('start', 'classId'), 'Slot')

    @async_tst
    async def test_classId_node(self):
        s = await getSchema("MyDevice")
        self.assertFalse(s.hash.hasAttribute('nodeOutput', 'classId'),
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
            await d.nodeWithSlot.pressMe()
            self.assertEqual(self.myDevice.lastCommand, "nodeWithSlot.pressMe")
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
    async def test_allowed_state_reconfigure_nodes(self):
        with (await getDevice("MyDevice")) as d:
            self.assertEqual(d.noded.floatProperty, 10.0)
            await setWait(d, 'noded.floatProperty', 27.0)
            self.assertEqual(d.noded.floatProperty, 27.0)
            await setWait(d, 'noded.floatProperty', 10.0)
            self.assertEqual(d.noded.floatProperty, 10.0)
            with self.assertRaises(KaraboError):
                await setWait(d, 'noded.intProperty', 22)
            self.assertEqual(d.noded.intProperty, 7)

    @sync_tst
    def test_slot_verification(self):
        self.assertEqual(self.myDevice.slotHasSlot("increaseCounter"), True)
        self.assertEqual(self.myDevice.slotHasSlot("output"), False)
        self.assertEqual(self.myDevice.slotHasSlot("doesNotExist"), False)

    @async_tst
    async def test_instance_info(self):
        device = self.myDevice
        self.assertEqual(device._ss.info["status"], "ok")
        await device.setState(State.ERROR)
        await sleep(0)
        self.assertEqual(device._ss.info["status"], "error")
        await device.setState(State.UNKNOWN)
        await sleep(0)
        self.assertEqual(device._ss.info["status"], "unknown")
        await device.setState(State.ON)
        await sleep(0)
        self.assertEqual(device._ss.info["status"], "ok")

    @async_tst
    async def test_output_information(self):
        device = self.myDevice
        # Second argument processId is not used in MDL
        success, data = device.slotGetOutputChannelInformation(
            "output", None)
        self.assertEqual(success, True)
        self.assertEqual(data["connectionType"], "tcp")
        self.assertEqual(data["memoryLocation"], "remote")
        self.assertIsInstance(data["port"], np.uint32)

        success, data = device.slotGetOutputChannelInformation(
            "doesNotExist", None)
        self.assertEqual(success, False)
        self.assertEqual(data, Hash())

    @async_tst
    async def test_output_information_hash_version(self):
        # tests the version that the GUI can generically call
        device = self.myDevice
        info = Hash('channelId', 'output')
        h = device.slotGetOutputChannelInformationFromHash(info)
        success, data = h["success"], h["info"]
        self.assertEqual(success, True)
        self.assertEqual(data["connectionType"], "tcp")
        self.assertEqual(data["memoryLocation"], "remote")
        self.assertIsInstance(data["port"], np.uint32)

        info = Hash('channelId', 'doesNotExist')
        h = device.slotGetOutputChannelInformationFromHash(info)
        success, data = h["success"], h["info"]
        self.assertEqual(success, False)
        self.assertEqual(data, Hash())

        info = Hash('NoChannelId', 'NotImportant')
        with self.assertRaises(KeyError):
            device.slotGetOutputChannelInformationFromHash(info)

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
    async def test_get_property_set_property(self):
        prop = get_property(self.myDevice, "integer")
        set_property(self.myDevice, "integer", 0)
        self.assertEqual(prop, 0)
        set_property(self.myDevice, "noded.floatProperty", 10)
        prop = get_property(self.myDevice, "noded.floatProperty")
        self.assertEqual(prop, 10)
        set_property(self.myDevice, "noded.floatProperty", 15)
        self.assertIsNot(prop, self.myDevice.noded.floatProperty)
        prop = get_property(self.myDevice, "noded.floatProperty")
        self.assertEqual(prop, 15)
        with self.assertRaises(AttributeError):
            get_property(self.myDevice, "noded.notthere")
        with self.assertRaises(AttributeError):
            get_property(self.myDevice, "notthere")
        with self.assertRaises(AttributeError):
            set_property(self.myDevice, "notthere", 25)
        with self.assertRaises(AttributeError):
            set_property(self.myDevice, "noded.notthere", 2)

    @async_tst
    async def test_get_properties_hash(self):
        self.myDevice.integer = 0

        # Remote slot call via slotGetConfigurationSlice
        deviceId = self.myDevice.deviceId
        h = await getProperties(deviceId, "integer")
        self.assertIsInstance(h, Hash)
        self.assertEqual(h["integer"], 0)
        self.assertEqual(len(h.paths(intermediate=False)), 1)

        slot = "slotGetConfigurationSlice"
        r = await get_instance().call(
            deviceId, slot, Hash("paths", ["integer"]))
        self.assertTrue(r.fullyEqual(h))

        # And we have the timestamp
        attrs = h["integer", ...]
        self.assertEqual(len(attrs), 3)
        self.assertIn("tid", attrs)
        self.assertIn("frac", attrs)
        self.assertIn("sec", attrs)

        set_property(self.myDevice, "noded.floatProperty", 15)
        h = await getProperties(deviceId,
                                ["integer", "noded.floatProperty"])
        self.assertEqual(h["integer"], 0)
        self.assertEqual(h["noded.floatProperty"], 15)
        self.assertEqual(len(h.paths()), 3)
        self.assertEqual(len(h.paths(intermediate=False)), 2)

        # exception for failing properties
        with self.assertRaises(KaraboError):
            h = await getProperties(deviceId, ["integer", "noded.notthere"])

        # KaraboError for failing garbage
        for fail in (
            [False, True],
            [Hash()],
            [1, 2],
            [1.78],
        ):
            with self.assertRaises(KaraboError):
                h = await getProperties(deviceId, fail)

        # Working with proxy
        async with getDevice(self.myDevice.deviceId) as proxy:
            h = await getProperties(proxy, ["integer", "noded.floatProperty"])
            self.assertEqual(h["integer"], 0)
            self.assertEqual(h["noded.floatProperty"], 15)
            self.assertEqual(len(h.paths(intermediate=False)), 2)

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

    @async_tst
    async def test_initialization(self):
        # this will not throw in the preInitialization
        bob = MyDevice({"_deviceId_": "bob", "preInitError": True})
        # fails in preInitialization -> instantiation will throw
        with self.assertRaisesRegex(RuntimeError, "I am going down"):
            await bob.startInstance()
        self.assertTrue(bob.isDown)
        self.assertFalse(bob.is_initialized)

        # this will not throw in the onInitialization
        alice = MyDevice({"_deviceId_": "alice", "initError": True})
        await alice.startInstance()
        # for backward compatibility, we kill the device on initError.
        self.assertTrue(alice.isDown)
        self.assertFalse(alice.is_initialized)

        # this will succeed
        charlie = MyDevice({"_deviceId_": "charlie"})
        await charlie.startInstance()
        self.assertFalse(charlie.isDown)
        self.assertTrue(charlie.is_initialized)
        await charlie.slotKillDevice()

    @async_tst
    async def test_preinitialization(self):
        """Test the preinitialization taking too long"""
        class ASlowInitDevice(Device):

            async def preInitialization(self):
                await sleep(10)

            async def slotKillDevice(self):
                self.isDown = True
                await super().slotKillDevice()

        charlie = ASlowInitDevice({"_deviceId_": "charlie"})
        with self.assertRaises(TimeoutError):
            await charlie.startInstance()
        self.assertTrue(charlie.isDown)
        self.assertFalse(charlie.is_initialized)

        class SlowInitDevice(Device):

            def preInitialization(self):
                mdlsleep(10)

            async def slotKillDevice(self):
                self.isDown = True
                await super().slotKillDevice()

        echo = SlowInitDevice({"_deviceId_": "echo"})
        with self.assertRaises(TimeoutError):
            await echo.startInstance()
        self.assertTrue(echo.isDown)
        self.assertFalse(echo.is_initialized)

    @async_tst
    async def test_atimer_destruct(self):

        global counter
        counter = 0

        class TimerDevice(Device):
            __version__ = "1.2.3"

            async def onInitialization(self):
                self.timer = AsyncTimer(self.timer_callback, 0.1)
                self.timer.start()
                get_event_loop().something_changed()

            async def timer_callback(self):
                global counter
                counter += 1

        device = TimerDevice({"_deviceId_": "timerDeviceTest"})
        await device.startInstance()
        self.assertEqual(counter, 0)
        await waitUntil(lambda: device.is_initialized)
        await sleep(0.2)
        self.assertGreater(counter, 0)
        await device.slotKillDevice()
        old_counter = counter
        await sleep(0.2)
        self.assertEqual(counter, old_counter)

        counter = 0
        device = TimerDevice({"_deviceId_": "timerDeviceTest"})
        await device.startInstance()
        await waitUntil(lambda: device.is_initialized)
        await sleep(0.2)
        self.assertGreater(counter, 0)
        device.__del__()
        await sleep(0.2)
        old_counter = counter
        await sleep(0.2)
        self.assertEqual(counter, old_counter)

        counter = 0
        device = TimerDevice({"_deviceId_": "timerDeviceTest"})
        await device.startInstance()
        await waitUntil(lambda: device.is_initialized)
        await device.slotKillDevice()
        self.assertEqual(0, counter)


if __name__ == '__main__':
    main()
