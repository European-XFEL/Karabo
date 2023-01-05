"""This tests the communication between bound API and middlelayer API"""

import os
import shutil
import sys
import time
from asyncio import (
    TimeoutError, create_subprocess_exec, ensure_future, get_event_loop,
    wait_for)
from contextlib import contextmanager
from datetime import datetime
from subprocess import PIPE
from unittest import main

import numpy as np

from karabo.common.enums import Capabilities, Interfaces
from karabo.middlelayer import (
    AccessLevel, AlarmCondition, Assignment, Configurable, DaqDataType,
    DeviceClientBase, Hash, Image, InputChannel, Int32, KaraboError,
    MetricPrefix, NDArray, Node, OutputChannel, Slot, State, String, UInt32,
    Unit, VectorDouble, background, call, encodeXML, getDevice, getHistory,
    isSet, setWait, shutdown, sleep, unit, updateDevice, waitUntil,
    waitUntilNew)
from karabo.middlelayer.testing import DeviceTest, async_tst, sleepUntil


class Child(Configurable):
    number = Int32()


class DataNode(Configurable):
    daqDataType = DaqDataType.TRAIN

    image = Image(
        shape=(10, 10),
        dtype=UInt32,
        displayedName="Image")

    array = NDArray(
        displayedName="NDArray",
        dtype=UInt32,
        shape=(10, 10))


class ImageChannel(Configurable):
    data = Node(DataNode)


class MiddlelayerDevice(DeviceClientBase):
    channelcount = 0
    rawchannelcount = 0

    value = Int32()

    channelclose = String(
        defaultValue="")

    boundDevice = String(
        defaultValue="")

    child = Node(Child)

    vectorMaxSize = VectorDouble(
        defaultValue=[2.0, 2.0],
        minSize=2, maxSize=4)

    array = NDArray(
        dtype=UInt32,
        shape=(2,))

    foundCapabilities = Int32(
        defaultValue=0)

    foundInterfaces = Int32(
        defaultValue=0)

    @Slot()
    async def slot(self):
        self.marker = True

    @InputChannel()
    async def channel(self, data, meta):
        self.channelcount += 1
        self.channeldata = data
        self.channelmeta = meta

    @channel.close
    async def channel(self, output):
        self.channelclose = output

    @InputChannel(raw=True)
    def rawchannel(self, data, meta):
        self.rawchannelcount += 1
        self.rawchanneldata = data
        self.rawchannelmeta = meta

    output = OutputChannel(Child)
    imageOutput = OutputChannel(ImageChannel)
    rawoutput = OutputChannel()

    @Slot()
    async def retrieveInterfaces(self):
        # Get the device topology
        device_topology = self.systemTopology["device"]
        capa = device_topology[self.boundDevice, "capabilities"]
        interfaces = device_topology[self.boundDevice, "interfaces"]
        # Add to our device and immediately update!
        self.foundCapabilities = capa
        self.foundInterfaces = interfaces
        self.update()

    @Slot()
    async def sendOutputEOS(self):
        await self.output.writeEndOfStream()

    @Slot()
    async def sendImageOutput(self):
        array = np.random.randint(1000, size=(10, 10), dtype=np.uint32)
        self.imageOutput.schema.data.array = array
        self.imageOutput.schema.data.image = array
        await self.imageOutput.writeData()


class Tests(DeviceTest):
    __loggerMap = "loggermap.xml"

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
        self.__starting_dir = os.getcwd()
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        # The cpp server always changes into "var/data" and logger manager
        # places its file into current directory - but we do not want to
        # interfere our test with what a developer had there before.
        loggerMap = "{}/var/data/{}".format(os.environ["KARABO"],
                                            self.__loggerMap)
        if os.path.exists(loggerMap):
            shutil.move(loggerMap, os.getcwd())
        # Test the middlelayer - bound pipeline interface
        self.process = None

    def tearDown(self):
        # Tear down our started bound devices
        had_to_kill = False
        if self.process is not None and self.process.returncode is None:
            self.process.kill()
            self.loop.run_until_complete(self.process.wait())
            had_to_kill = True

        # Place back logger map if copied
        if os.path.exists(self.__loggerMap):
            karaboRunDir = "{}/var/data".format(os.environ["KARABO"])
            shutil.copy2(self.__loggerMap, karaboRunDir)
            os.remove(self.__loggerMap)

        os.chdir(self.__starting_dir)
        if had_to_kill:
            self.fail("process didn't properly go down")

    @async_tst(timeout=90)
    async def test_cross(self):
        await getDevice("middlelayerDevice")
        # it takes typically 2 s for the bound device to start
        self.process = await create_subprocess_exec(
            sys.executable, "-m", "karabo.bound_api.launcher",
            "run", "karabo.bound_device_test", "TestDevice",
            stdin=PIPE, stderr=PIPE, stdout=PIPE)
        # Logging set to FATAL to swallow the misleading ERROR that comes
        # if we try (and fail as expected) to set a readOnly property on
        # For debugging you may switch to INFO or DEBUG.
        self.process.stdin.write(b"""\
            <root KRB_Artificial="">
                <_deviceId_>boundDevice</_deviceId_>
                <Logger><priority>FATAL</priority></Logger>
                <middlelayerDevice>middlelayerDevice</middlelayerDevice>
                <input>
                    <connectedOutputChannels>
                        middlelayerDevice:output,middlelayerDevice:rawoutput
                    </connectedOutputChannels>
                </input>
            </root>""")
        self.process.stdin.close()

        proxy = await getDevice("boundDevice")
        self.assertEqual(proxy.a, 22.5 * unit.milliampere,
                         "didn't receive inital value from bound device")
        self.assertEqual(proxy.readonly, 2,
                         "didn't receive initial value from bound device")

        # Basic Interfaces Check AFTER bound device is up
        # -----------------------------------------------
        mdl_proxy = await getDevice("middlelayerDevice")

        with mdl_proxy:
            await setWait(mdl_proxy, boundDevice="boundDevice")
            self.assertEqual(mdl_proxy.boundDevice, "boundDevice")
            await mdl_proxy.retrieveInterfaces()
            capas = mdl_proxy.foundCapabilities.value
            interfaces = mdl_proxy.foundInterfaces.value
            self.assertEqual(capas, Capabilities.PROVIDES_INTERFACES)
            interface_detail = (Interfaces.Motor + Interfaces.Camera +
                                Interfaces.Processor +
                                Interfaces.DeviceInstantiator)
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
            has_instantiator = _test_mask(interfaces,
                                          Interfaces.DeviceInstantiator)
            self.assertTrue(has_instantiator)

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
        self.assertEqual(proxy.table[0]["d"], 5 * unit.kilometer)

        with proxy:
            self.assertEqual(proxy.maxSizeSchema, 0)
            # Test the maxSize from a vector property!
            # and the reply of a slot.
            reply = await proxy.compareSchema()
            self.assertEqual(proxy.maxSizeSchema, 4)
            self.assertEqual(reply, 4)

            await proxy.setA()
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
                await setWait("boundDevice", readonly=1)  # not allowed
            self.assertEqual(proxy.readonly, 2)  # unchanged

            def setter():
                proxy.a = 22.3 * unit.milliampere
                self.assertEqual(proxy.a, 22.3 * unit.milliampere)

            await background(setter)

            proxy.a = 0.0228 * unit.ampere
            self.assertNotEqual(proxy.a, 22.8 * unit.milliampere,
                                "proxy should set device's, not own value")
            await waitUntil(lambda: proxy.a == 22.8 * unit.milliampere)
            self.assertEqual(proxy.a, 22.8 * unit.milliampere,
                             "didn't receive change from bound device")

            proxy.table = [(3, "african"), (7, "european")]
            self.assertEqual(len(proxy.table), 1)
            await waitUntilNew(proxy.table)
            self.assertEqual(len(proxy.table), 2)
            self.assertEqual(proxy.table[1]["d"], 7 * unit.kilometer)
            self.assertEqual(proxy.table[0]["s"], "african")

            await proxy.injectSchema()
            self.assertEqual(proxy.word1, "Hello")
            self.assertEqual(proxy.word1.descriptor.description, "The word")
            self.assertEqual(proxy.injectedNode.number1, 1)

        await proxy.injectSchema()
        await updateDevice(proxy)
        self.assertEqual(proxy.word2, "Hello")
        self.assertEqual(proxy.injectedNode.number2, 2)

        await proxy.backfire()
        await sleep(1)  # See FIXME in backfire slot of the bound device
        self.assertEqual(self.device.value, 99)
        self.assertTrue(self.device.marker)

        ####################################
        # check slotGetTime of bound device
        ####################################
        hTime = await call("boundDevice", "slotGetTime", Hash())
        self.assertIsNotNone(hTime)
        self.assertTrue(hTime["time"])
        sec = hTime.getAttributes("time")["sec"]
        frac = hTime.getAttributes("time")["frac"]
        t1 = sec + frac / 1.e18
        # non-zero time, but zero tid
        self.assertGreater(t1, 0)
        self.assertEqual(hTime.getAttributes("time")["tid"], 0)
        # Now send tid info to device (1559600000 is June 3rd, 2019)
        await call("boundDevice", "slotTimeTick",
                   # id sec       frac periodInMicroSec
                   100, 1559600000, 0, 100000)
        # ask again, now non-zero tid and a later point in time
        hTime = await call("boundDevice", "slotGetTime", Hash())
        sec = hTime.getAttributes("time")["sec"]
        frac = hTime.getAttributes("time")["frac"]
        t2 = sec + frac / 1.e18
        self.assertGreater(t2, t1)
        self.assertGreaterEqual(hTime.getAttributes("time")["tid"], 100)

        ####################################
        # pipeline part
        ####################################
        await proxy.send()
        # XXX: This was working without any wait or sleep in jms
        await sleepUntil(lambda: self.device.channelcount == 1)
        self.assertEqual(self.device.channelcount, 1)
        self.assertFalse(isSet(self.device.channeldata.d))
        self.assertEqual(self.device.channeldata.s, "hallo")
        self.assertEqual(self.device.channelmeta.source, "boundDevice:output1")
        self.assertTrue(self.device.channelmeta.timestamp)
        self.assertEqual(self.device.rawchannelcount, 1)

        self.assertEqual(self.device.rawchannelmeta.source,
                         "boundDevice:output2")
        self.assertEqual(self.device.rawchanneldata["e"], 5)
        self.assertEqual(self.device.rawchanneldata["s"], "hallo")
        ndarray_hsh = self.device.rawchanneldata["ndarray"]
        self.assertTrue(ndarray_hsh is not None)
        # the incoming ndarray is of shape (10) and type np.float32
        self.assertEqual(len(ndarray_hsh['data']), 10 * 4)
        image_hsh = self.device.rawchanneldata["image"]
        self.assertTrue(image_hsh is not None)
        # the incoming image is of shape (50, 50) and type np.uint16
        self.assertEqual(len(image_hsh['pixels.data']), 50 * 50 * 2)
        self.assertTrue(self.device.rawchannelmeta.timestamp)

        await proxy.send()
        self.assertEqual(self.device.channelcount, 2)

        await proxy.end()
        # The end of stream is no longer setting the data to None
        self.assertEqual(self.device.channeldata.s, "hallo")
        self.assertEqual(self.device.channelmeta.source,
                         "boundDevice:output1")
        self.assertEqual(self.device.channelcount, 2)

        await proxy.send()
        self.assertEqual(self.device.channeldata.s, "hallo")
        self.assertEqual(self.device.channelmeta.source, "boundDevice:output1")
        self.assertEqual(self.device.channelcount, 3)

        proxy.output1.connect()
        task = background(waitUntilNew(proxy.output1.schema.s))
        while not task.done():
            # unfortunately, connecting takes time and nobody knows how much
            await proxy.send()
        await task
        self.assertEqual(proxy.output1.schema.s, "hallo")

        with proxy:
            # The await below fixes a race condition in the context initialization
            # performed by with. As soon as the MR with the 'async with' implementation
            # (MR !3210) is merged, the 'await updateDevice(proxy)' can be removed.
            await updateDevice(proxy)
            self.device.output.schema.number = 23
            self.assertEqual(proxy.a, 22.8 * unit.milliampere)
            await self.device.output.writeData()
            await waitUntil(lambda: proxy.a == 23 * unit.mA)

        proxy.output1.connect()
        task = background(waitUntilNew(proxy.output1.schema.s))
        while not task.done():
            # TODO: wait for connection by listening to `input.missingConnections`
            # unfortunately, connecting takes time and nobody knows how much
            await proxy.send()
        await task
        self.assertEqual(proxy.output1.schema.s, "hallo")

        ####################################
        # clean up
        ####################################
        await shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        await self.process.wait()
        # Wait a few seconds for the broker message
        await sleep(2)
        self.assertEqual(self.device.channelclose, "boundDevice:output1")

    @async_tst(timeout=90)
    async def test_cross_image(self):
        config = Hash(
            "Logger.priority", "FATAL",
            "_deviceId_", "boundDevice2",
            "deviceId", "boundDevice2",
            "middlelayerDevice", "middlelayerDevice",
            "imageInput.connectedOutputChannels", "middlelayerDevice:imageOutput",
            "imageInput.onSlowness", "wait",
            "imagePath", "data.image",
            "ndarrayPath", "data.array"
        )
        self.process = await create_subprocess_exec(
            sys.executable, "-m", "karabo.bound_api.launcher",
            "run", "karabo.bound_device_test", "TestDevice",
            stdin=PIPE, stdout=PIPE)
        self.process.stdin.write(encodeXML(config).encode('utf8'))
        self.process.stdin.close()
        # it takes typically 2 s for the bound device to start
        bound_proxy = await getDevice("boundDevice2")
        mdl_proxy = await getDevice("middlelayerDevice")

        ###################################################
        # Send data from middlelayer and receive from bound
        ###################################################
        with bound_proxy:
            # Failed with timeout=5
            # in https://git.xfel.eu/Karabo/Framework/-/jobs/289942
            try:
                await wait_for(
                    waitUntil(lambda: bound_proxy.imageInput.missingConnections == []),
                    timeout=15
                )
            except TimeoutError:
                assert False, "bound proxy did not connect in time"

            task = background(waitUntilNew(bound_proxy.imagesReceived))
            await mdl_proxy.sendImageOutput()
            try:
                await wait_for(task, timeout=15)
                assert task.done()
            except (AssertionError, TimeoutError):
                assert False, f"bound proxy did not update in time. status : {bound_proxy.status}"

            self.assertGreater(bound_proxy.imagesReceived, 0)
            self.assertGreater(bound_proxy.ndarraysReceived, 0)

        ####################################
        # clean up
        ####################################
        await shutdown(bound_proxy)
        await self.process.wait()

    @async_tst(timeout=90)
    async def test_history(self):
        before = datetime.now()
        # Wherever we run this test (by hands or in CI) we should
        # not depend on the exact location of 'historytest.xml' ...
        # ... so let's create it on the fly ...
        xml_path = os.getcwd() + '/historytest.xml'
        xml = open(xml_path, 'wb')
        xml.write(b"""\
<?xml version="1.0"?>
<DeviceServer>
  <autoStart>
    <KRB_Item>
      <DataLoggerManager>
        <!-- Frequent flushing of raw and index files every 1 s: -->
        <flushInterval KRB_Type="INT32">1</flushInterval>
        <logger>
            <FileDataLogger>
                <directory KRB_Type="STRING">karaboHistory</directory>
            </FileDataLogger>
        </logger>
        <serverList KRB_Type="VECTOR_STRING">karabo/dataLogger</serverList>
      </DataLoggerManager>
    </KRB_Item>
  </autoStart>
  <scanPlugins KRB_Type="STRING">false</scanPlugins>
  <serverId KRB_Type="STRING">karabo/dataLogger</serverId>
  <visibility>4</visibility>
  <Logger><priority>INFO</priority></Logger>
</DeviceServer>""")
        xml.close()

        server = "karabo/dataLogger"  # Also twice in xml text above

        # Use above configuration to start DataLoggerManager ...
        karabo = os.environ["KARABO"]
        self.process = await create_subprocess_exec(
            os.path.join(karabo, "bin", "karabo-cppserver"),
            xml_path,
            stdin=PIPE, stderr=PIPE, stdout=PIPE)

        async def print_stdout():
            while not self.process.stdout.at_eof():
                line = await self.process.stdout.readline()
                print(line.decode("ascii"), end="")

        ensure_future(print_stdout())

        try:
            # Wait until logger is ready for logging our middlelayer device,
            # i.e. published non-empty timestamp of any logged parameter
            with (await getDevice(
                    "DataLogger-{}".format(server))) as logger:
                await updateDevice(logger)
                while True:
                    found = False
                    for row in logger.lastUpdatesUtc:
                        if (row["deviceId"] == "middlelayerDevice"
                                and row["lastUpdateUtc"]):
                            found = True
                    if found:
                        break
                    await waitUntilNew(logger.lastUpdatesUtc)
        finally:
            os.remove(xml_path)

        # Initiate indexing for selected parameters: "value" and "child.number"
        after = datetime.now()

        # The first history request ever fails - but it triggers the
        # creation of index files needed later for reading back.
        # Sometimes this test runs in an environment where these requests
        # are not the first ones, so cannot assertRaise.
        try:
            await getHistory("middlelayerDevice.value",
                             before.isoformat(), after.isoformat())
        except KaraboError as e:
            self.assertIn("first history request", str(e))
        try:
            await getHistory("middlelayerDevice.child.number",
                             before.isoformat(), after.isoformat())
        except KaraboError as e:
            self.assertIn("first history request", str(e))

        # Now write data that is logged (and indexed for reading back!)
        for i in range(5):
            self.device.value = i
            self.device.child.number = -i
            self.device.update()

        await logger.flush()

        after = datetime.now()

        str_history = await getHistory(
            "middlelayerDevice.value", before.isoformat(), after.isoformat())
        device = await getDevice("middlelayerDevice")
        proxy_history = await getHistory(
            device.value, before.isoformat(), after.isoformat())

        for hist in str_history, proxy_history:
            # Sort according to timestamp - order is not guaranteed!
            # (E.g. if shortcut communication between logged device and
            #  logger is switched on...)
            hist.sort(key=lambda x: x[0])
            self.assertEqual([v for _, _, _, v in hist[-5:]], list(range(5)))

        node_history = await getHistory(
            "middlelayerDevice.child.number", before.isoformat(),
            after.isoformat())
        node_proxy_history = await getHistory(
            device.child.number, before.isoformat(), after.isoformat())

        for hist in node_history, node_proxy_history:
            # Sort needed - see above.
            hist.sort(key=lambda x: x[0])
            self.assertEqual([-v for _, _, _, v in hist[-5:]], list(range(5)))

        await get_event_loop().instance()._ss.request(
            server, "slotKillServer")
        await self.process.wait()

    test_history.slow = True


if __name__ == "__main__":
    main()
