"""This tests the communication between bound API and middlelayer API"""

from asyncio import (
    async, coroutine, create_subprocess_exec, get_event_loop)
from contextlib import contextmanager
from datetime import datetime
import os
import os.path
from subprocess import PIPE
import sys
from unittest import main

from karabo.common.enums import Capabilities, Interfaces
from karabo.middlelayer import (
    AccessLevel, AccessMode, AlarmCondition, Assignment, background, Configurable,
    DeviceClientBase, getDevice, getHistory, isSet, InputChannel,
    Int32, KaraboError, MetricPrefix, Node,
    OutputChannel, setWait, shutdown, sleep, Slot, State, String, unit, Unit,
    VectorDouble, Float, VectorString, VectorHash, Hash, waitUntil, waitUntilNew)

from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst


class Child(Configurable):
    number = Int32()

class Row(Configurable):
    x = Float(defaultValue=1.0)
    y = Float(defaultValue=1.0)

class MiddlelayerDevice(DeviceClientBase):
    channelcount = 0
    rawchannelcount = 0
    value = Int32()

    boundDevice = String(defaultValue="")
    child = Node(Child)

    vectorMaxSize = VectorDouble(defaultValue=[2.0, 2.0],
                                 minSize=2, maxSize=4)

    foundCapabilities = Int32(
        defaultValue=0)

    foundInterfaces = Int32(
        defaultValue=0)

    vectorString = VectorString(defaultValue=['abc','fgi','abrakadabra','23456PQ','xyz'])

    table = VectorHash(rows=Row, defaultValue=[Hash("x", 2.0, "y", 5.6)], accessMode=AccessMode.RECONFIGURABLE)


    @Slot()
    @coroutine
    def slot(self):
        self.marker = True

    @InputChannel()
    @coroutine
    def channel(self, data, meta):
        self.channelcount += 1
        self.channeldata = data
        self.channelmeta = meta

    @channel.close
    @coroutine
    def channel(self, output):
        self.channelclose = output

    @InputChannel(raw=True)
    def rawchannel(self, data, meta):
        self.rawchannelcount += 1
        self.rawchanneldata = data
        self.rawchannelmeta = meta

    output = OutputChannel(Child)
    rawoutput = OutputChannel()

    @Slot()
    @coroutine
    def retrieveInterfaces(self):
        # Get the device topology
        device_topology = self.systemTopology["device"]
        capa = device_topology[self.boundDevice, "capabilities"]
        interfaces = device_topology[self.boundDevice, "interfaces"]
        # Add to our device and immediately update!
        self.foundCapabilities = capa
        self.foundInterfaces = interfaces
        self.update()


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.device = MiddlelayerDevice(dict(
            _deviceId_="middlelayerDevice",
            rawchannel=dict(connectedOutputChannels=["boundDevice:output2"],
                            onSlowness="drop"),
            channel=dict(connectedOutputChannels=["boundDevice:output1"])))
        with cls.deviceManager(lead=cls.device):
            yield

    def setUp(self):
        self.__starting_dir = os.curdir
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        # Test the middlelayer - bound pipeline interface
        self.process = None

    def tearDown(self):
        # Tear down our started bound devices
        had_to_kill = False
        if self.process is not None and self.process.returncode is None:
            self.process.kill()
            self.loop.run_until_complete(self.process.wait())
            had_to_kill = True

        for fn in ("karabo.log", "serverId.xml", "loggermap.xml"):
            try:
                os.remove(fn)
            except FileNotFoundError:
                pass  # never mind

        os.chdir(self.__starting_dir)
        if had_to_kill:
            self.fail("process didn't properly go down")

    @async_tst(timeout=90)
    def test_cross(self):
        # it takes typically 2 s for the bound device to start
        self.process = yield from create_subprocess_exec(
            sys.executable, "-m", "karabo.bound_api.launcher",
            "run", "karabo.bound_device_test", "TestDevice",
            stdin=PIPE)
        # To get DEBUG output of the bound device, add
        # <Logger><priority>DEBUG</priority></Logger>
        # after the line with _deviceId_
        self.process.stdin.write(b"""\
            <root KRB_Artificial="">
                <_deviceId_>boundDevice</_deviceId_>
                <input>
                    <connectedOutputChannels>
                        middlelayerDevice:output,middlelayerDevice:rawoutput
                    </connectedOutputChannels>
                </input>
            </root>""")
        self.process.stdin.close()

        proxy = yield from getDevice("boundDevice")
        self.assertEqual(proxy.a, 22.5 * unit.milliampere,
                         "didn't receive inital value from bound device")
        self.assertEqual(proxy.readonly, 2,
                         "didn't receive initial value from bound device")

        # Basic Interfaces Check AFTER bound device is up
        # -----------------------------------------------

        with (yield from getDevice("middlelayerDevice")) as mdl_proxy:
            yield from setWait(mdl_proxy, boundDevice="boundDevice")
            self.assertEqual(mdl_proxy.boundDevice, "boundDevice")
            yield from mdl_proxy.retrieveInterfaces()
            capas = mdl_proxy.foundCapabilities.value
            interfaces = mdl_proxy.foundInterfaces.value
            self.assertEqual(capas, Capabilities.PROVIDES_INTERFACES)
            interface_detail = (Interfaces.Motor + Interfaces.Camera
                                + Interfaces.Processor)
            self.assertEqual(interfaces, interface_detail)

            def _test_mask(mask, bit):
                return (mask & bit) == bit

            has_interfaces = _test_mask(capas,
                                        Capabilities.PROVIDES_INTERFACES)
            self.assertTrue(has_interfaces)

            has_motor = _test_mask(interfaces, Interfaces.Motor)
            self.assertTrue(has_motor)
            has_camera = _test_mask(interfaces, Interfaces.Camera)
            self.assertTrue(has_camera)
            has_processor = _test_mask(interfaces, Interfaces.Processor)
            self.assertTrue(has_processor)

        a_desc = type(proxy).a
        self.assertIs(a_desc.unitSymbol, Unit.AMPERE)
        self.assertIs(a_desc.metricPrefixSymbol, MetricPrefix.MILLI)
        self.assertIs(a_desc.requiredAccessLevel, AccessLevel.EXPERT)
        self.assertIs(a_desc.assignment, Assignment.OPTIONAL)
        self.assertEqual(a_desc.displayedName, "parameter a")
        self.assertEqual(a_desc.description, "a's description")
        self.assertEqual(a_desc.defaultValue, 22.5)
        self.assertEqual(a_desc.minExc, 22)
        self.assertEqual(a_desc.maxExc, 33)
        self.assertEqual(a_desc.minInc, 11)
        self.assertEqual(a_desc.maxInc, 23)
        self.assertEqual(a_desc.allowedStates, {State.INIT, State.UNKNOWN})
        self.assertEqual(a_desc.tags, {"bla", "blub"})

        self.assertEqual(len(proxy.table), 1)
        self.assertEqual(proxy.table[0]["d"], 5 * unit.meter)

        with proxy:
            self.assertEqual(proxy.maxSizeSchema, 0)
            # Test the maxSize from a vector property!
            proxy.middlelayerDevice = "middlelayerDevice"
            yield from proxy.compareSchema()
            self.assertEqual(proxy.maxSizeSchema, 4)

            yield from proxy.setA()
            self.assertEqual(proxy.a, 22.7 * unit.milliampere,
                             "didn't receive change from bound device")
            self.assertEqual(repr(proxy.a.timestamp),
                             "2009-09-01T13:55:22 UTC")
            self.assertEqual(proxy.node.b, 100 * unit.kilometer)
            self.assertEqual(repr(proxy.node.b.timestamp),
                             "2016-06-17T13:55:22 UTC")
            self.assertEqual(proxy.state, State.UNKNOWN)
            self.assertEqual(proxy.alarmCondition, AlarmCondition.NONE)

            self.assertEqual(proxy.ndarray[0, 1], 2 * unit.kilometer)
            self.assertEqual(proxy.ndarray.shape, (2, 3))
            self.assertEqual(proxy.ndarray.descriptor.unitSymbol, Unit.METER)
            self.assertEqual(proxy.ndarray.descriptor.metricPrefixSymbol,
                             MetricPrefix.KILO)
            self.assertEqual(proxy.ndarray.dtype.str, "<i8")

            with self.assertRaises(ValueError):
                proxy.a = 77
            self.assertEqual(proxy.a, 22.7 * unit.milliampere)

            # Following test does not yet work, but in fact the request to
            # change readonly goes over the wire (but shouldn't) and is refused
            # on the other end which sends back an error...
            # with self.assertRaises(ValueError):  # or KaraboError?
            #    proxy.readonly = 1  # not allowed!
            with self.assertRaises(KaraboError):
                yield from setWait("boundDevice", readonly=1)  # not allowed
            self.assertEqual(proxy.readonly, 2)  # unchanged

            def setter():
                proxy.a = 22.3 * unit.milliampere
                self.assertEqual(proxy.a, 22.3 * unit.milliampere)

            yield from background(setter)

            proxy.a = 0.0228 * unit.ampere
            self.assertNotEqual(proxy.a, 22.8 * unit.milliampere,
                                "proxy should set device's, not own value")
            yield from waitUntil(lambda: proxy.a == 22.8 * unit.milliampere)
            self.assertEqual(proxy.a, 22.8 * unit.milliampere,
                             "didn't receive change from bound device")

            proxy.table = [(3, "african"), (7, "european")]
            self.assertEqual(len(proxy.table), 1)
            yield from waitUntilNew(proxy.table)
            self.assertEqual(len(proxy.table), 2)
            self.assertEqual(proxy.table[1]["d"], 7 * unit.meter)
            self.assertEqual(proxy.table[0]["s"], "african")

            yield from proxy.injectSchema()
            self.assertEqual(proxy.word1, "Hello")
            self.assertEqual(proxy.word1.descriptor.description, "The word")
            self.assertEqual(proxy.injectedNode.number1, 1)

        yield from proxy.injectSchema()
        yield from proxy
        self.assertEqual(proxy.word2, "Hello")
        self.assertEqual(proxy.injectedNode.number2, 2)

        yield from proxy.backfire()
        self.assertEqual(self.device.value, 99)
        self.assertTrue(self.device.marker)

        yield from proxy.send()
        self.assertEqual(self.device.channelcount, 1)
        self.assertFalse(isSet(self.device.channeldata.d))
        self.assertEqual(self.device.channeldata.s, "hallo")
        self.assertEqual(self.device.channelmeta.source, "boundDevice:output1")
        self.assertTrue(self.device.channelmeta.timestamp)
        self.assertEqual(self.device.rawchannelcount, 1)
        self.assertEqual(self.device.rawchanneldata["e"], 5)
        self.assertEqual(self.device.rawchanneldata["s"], "hallo")
        self.assertEqual(self.device.rawchannelmeta.source,
                         "boundDevice:output2")
        self.assertTrue(self.device.rawchannelmeta.timestamp)

        yield from proxy.send()
        self.assertEqual(self.device.channelcount, 2)
        yield from proxy.end()
        # The end of stream is no longer setting the data to None
        self.assertEqual(self.device.channeldata.s, "hallo")
        self.assertEqual(self.device.channelmeta.source,
                         "boundDevice:output1")
        self.assertEqual(self.device.channelcount, 2)
        yield from proxy.send()
        self.assertEqual(self.device.channeldata.s, "hallo")
        self.assertEqual(self.device.channelmeta.source, "boundDevice:output1")
        self.assertEqual(self.device.channelcount, 3)

        proxy.output1.connect()
        task = background(waitUntilNew(proxy.output1.schema.s))
        while not task.done():
            # unfortunately, connecting takes time and nobody knows how much
            yield from proxy.send()
        yield from task
        self.assertEqual(proxy.output1.schema.s, "hallo")

        with proxy:
            self.device.output.schema.number = 23
            yield from self.device.output.writeData()
            yield from waitUntil(lambda: proxy.a == 23 * unit.mA)

        proxy.output1.connect()
        task = background(waitUntilNew(proxy.output1.schema.s))
        while not task.done():
            # unfortunately, connecting takes time and nobody knows how much
            yield from proxy.send()
        yield from task
        self.assertEqual(proxy.output1.schema.s, "hallo")

        yield from shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        yield from self.process.wait()
        self.assertEqual(self.device.channelclose, "boundDevice:output1")

    @async_tst(timeout=90)
    def test_history(self):
        before = datetime.now()

        karabo = os.environ["KARABO"]
        xml = os.path.abspath('historytest.xml')
        self.process = yield from create_subprocess_exec(
            os.path.join(karabo, "bin", "karabo-cppserver"),
            xml, stderr=PIPE, stdout=PIPE)

        @coroutine
        def print_stdout():
            while not self.process.stdout.at_eof():
                line = yield from self.process.stdout.readline()
                print(line.decode("ascii"), end="")

        async(print_stdout())

        with (yield from getDevice("DataLogger-middlelayerDevice")) as logger:
            yield from logger
            yield from waitUntil(lambda: logger.state == State.NORMAL)

        for i in range(4):
            self.device.value = i
            self.device.child.number = -i
            self.device.table[0] = (7.77 * i,-15.55 * i)
            self.device.update()

        after = datetime.now()

        # This is the first history request ever, so it returns an empty
        # list (see https://in.xfel.eu/redmine/issues/9414).
        yield from getHistory(
            "middlelayerDevice.value", before.isoformat(), after.isoformat())
        yield from getHistory(
            "middlelayerDevice.child.number", before.isoformat(),
            after.isoformat())

        # We have to write another value to close the first archive file :-(...
        self.device.value = 4
        self.device.child.number = -4
        self.device.table[0] = (7.77 * 4, -15.55 * 4)
        self.device.update()

        # ... and finally need to wait until the new archive and index files
        # are flushed (see flushInterval in history.xml).
        yield from sleep(1.1)

        after = datetime.now()

        old_history = yield from getHistory(
            "middlelayerDevice", before.isoformat(), after.isoformat()).value
        str_history = yield from getHistory(
            "middlelayerDevice.value", before.isoformat(), after.isoformat())
        device = yield from getDevice("middlelayerDevice")
        proxy_history = yield from getHistory(
            device.value, before.isoformat(), after.isoformat())

        for hist in old_history, str_history, proxy_history:
            # Sort according to timestamp - order is not guaranteed!
            # (E.g. if shortcut communication between logged device and
            #  logger is switched on...)
            hist.sort(key=lambda x: x[0])
            self.assertEqual([v for _, _, _, v in hist[-5:]], list(range(5)))

        node_history = yield from getHistory(
            "middlelayerDevice.child.number", before.isoformat(),
            after.isoformat())
        node_proxy_history = yield from getHistory(
            device.child.number, before.isoformat(), after.isoformat())

        for hist in node_history, node_proxy_history:
            # Sort needed - see above.
            hist.sort(key=lambda x: x[0])
            self.assertEqual([-v for _, _, _, v in hist[-5:]], list(range(5)))

        yield from get_event_loop().instance()._ss.request(
            "karabo/dataLogger", "slotKillServer")
        yield from self.process.wait()

    test_history.slow = True


if __name__ == "__main__":
    main()
