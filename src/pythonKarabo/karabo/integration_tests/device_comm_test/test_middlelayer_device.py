from asyncio import async, coroutine
from contextlib import contextmanager
import gc
import os
import os.path
from textwrap import dedent
from unittest import TestCase
import weakref

from karabo.middlelayer import (
    connectDevice, Device, getClasses, getDevice, getDevices, getServers,
    instantiate, isAlive, KaraboError, Macro, shutdown, sleep, Slot)
from karabo.middlelayer_api.cli import DeviceClient
from karabo.middlelayer_api.device_server import DeviceServer
from karabo.middlelayer_api.tests.eventloop import setEventLoop


class Remote(Macro):
    @Slot()
    def instantiate(self):
        instantiate("testServer", "MiddleLayerTestDevice", "other",
                    counter=12345)

    @Slot()
    def shutdown(self):
        shutdown("other")


class Tests(TestCase):
    def setUp(self):
        # Change the current directory to the directory containing this test.
        # DeviceServer looks for a file named serverId.xml in the current
        # directory, so we should use the one that comes with the unit tests
        # to avoid creating a new one in a random part of the user's filesystem
        self.__starting_dir = os.curdir
        this_dir = os.path.dirname(os.path.abspath(__file__))
        os.chdir(this_dir)
        self.loop = setEventLoop()

        self.dc = DeviceClient(dict(_deviceId_="dc"))
        self.dc.startInstance()

        dirname = os.path.dirname(__file__)
        self.server = DeviceServer(
            dict(serverId="testServer", pluginDirectory=dirname))
        self.loop.run_until_complete(self.server.startInstance())

    def tearDown(self):
        self.loop.close()
        try:
            os.remove("serverId.xml")
        except FileNotFoundError:
            pass
        os.chdir(self.__starting_dir)

    def run_async(self, device, coro):
        """run *coro* on behalf of *device*"""
        return self.loop.run_until_complete(
            self.loop.create_task(coro, device))

    @coroutine
    def init_server(self):
        """initialize the device server and a device on it

        the device on the device server is started from within a
        macro `remote`, such that we can test device instantiation
        from macros."""
        remote = Remote(_deviceId_="remote")
        yield from remote.startInstance()
        proxy = yield from getDevice("remote")
        yield from proxy.instantiate()
        return (yield from getDevice("other"))

    @coroutine
    def shutdown_server(self):
        """shutdown the device started by `init_server`

        Again a macro is called to shut down the device on the server,
        to test device shutdown in macros"""
        proxy = yield from getDevice("remote")
        yield from proxy.shutdown()
        yield from sleep(0.1)

    @coroutine
    def check_server_topology(self):
        schema, classId, serverId = yield from self.dc.call(
            "testServer", "slotGetClassSchema", "MiddleLayerTestDevice")
        self.assertEqual(serverId, "testServer")
        self.assertEqual(classId, "MiddleLayerTestDevice")
        self.assertEqual(schema.name, "MiddleLayerTestDevice")
        h = schema.hash
        self.assertEqual(h["counter", ...], {
            'alarmInfo_alarmHigh': '',
            'alarmInfo_warnHigh': '',
            'alarmInfo_alarmLow': '',
            'alarmInfo_warnLow': '',
            'alarmNeedsAck_alarmHigh': False,
            'alarmNeedsAck_warnHigh': False,
            'alarmNeedsAck_alarmLow': False,
            'alarmNeedsAck_warnLow': False,
            'accessMode': 4,
            'assignment': 0,
            'defaultValue': -1,
            'metricPrefixSymbol': '',
            'nodeType': 0,
            'requiredAccessLevel': 0,
            'unitSymbol': '',
            'valueType': 'INT32'})

        self.assertIn("testServer", getServers())
        self.assertIn("other", getDevices("testServer"))
        self.assertIn("MiddleLayerTestDevice", getClasses("testServer"))
        self.assertNotIn("SomeBlupp", getClasses("testServer"))

    def test_server(self):
        """test the full lifetime of a Python device server

        this test
          * starts a device client
          * starts a device server
          * starts a macro which tells said device server to start a device
          * checks that the other device shows up in the topology
          * let the macro tell to shut down that device
          * checks everything is cleaned up afterwards
          * kills the device server
        """
        proxy = self.run_async(self.server, self.init_server())
        self.assertEqual(proxy.something, 222,
                         "MiddleLayerTestDevice.onInitialization "
                         "did not run properly")
        self.assertEqual(proxy.counter, 12345,
                         "initialization parameter was not honored")

        self.run_async(self.dc, self.check_server_topology())
        self.assertIn("other", self.server.deviceInstanceMap)
        r = weakref.ref(self.server.deviceInstanceMap["other"])
        with proxy:
            self.assertTrue(isAlive(proxy))
            self.run_async(self.server, self.shutdown_server())
            self.assertNotIn("other", self.server.deviceInstanceMap)
            self.assertFalse(isAlive(proxy))
            gc.collect()
            self.assertIsNone(r())
            async(self.dc.slotKillDevice())
            self.loop.run_until_complete(self.server.slotKillServer())
            self.assertEqual(proxy.something, 111)
        self.loop.run_forever()

    @contextmanager
    def temp_file(self, path, content):
        f = open(path, "w")
        f.write(dedent(content))
        f.close()
        try:
            yield
        finally:
            os.remove(path)

    def test_bound(self):
        with self.assertRaises(KaraboError):
            # mandatory expected parameter "remote" is missing
            self.run_async(self.dc, instantiate("testServer", "CommTestDevice",
                                                "commtestdevice"))
        self.run_async(self.dc, instantiate("testServer", "CommTestDevice",
                                            "commtestdevice", remote="asdf"))
        self.run_async(self.dc, shutdown("commtestdevice"))

    def test_connect_during_initialization(self):
        class TestDevice(Device):
            @coroutine
            def onInitialization(self):
                self.remote = yield from connectDevice("commtestdevice")
        self.run_async(self.dc, instantiate("testServer", "CommTestDevice",
                                            "commtestdevice", remote="asdf"))
        try:
            device = TestDevice({"_deviceId_": "testdevice"})
            self.loop.run_until_complete(device.startInstance())
            self.run_async(device, device.remote.slotWithoutArguments())
            self.assertEqual(device.remote.someString,
                             "slotWithoutArguments was called")
        finally:
            self.run_async(self.dc, shutdown("commtestdevice"))

    def test_appearing_plugin(self):
        """test that new plugins appear in device server"""
        with self.assertRaises(KaraboError):
            self.run_async(self.dc, instantiate("testServer",
                                                "SomeDevice", "someName"))

        dirname = os.path.join(os.path.dirname(__file__), "Temporary.egg-info")
        os.mkdir(dirname)
        try:
            entry_points = self.temp_file(
                os.path.join(dirname, "entry_points.txt"), """
                    [karabo.middlelayer_device]
                    SomeDevice = middlelayertestdevice:SomeDevice
                """)
            pkg_info = self.temp_file(
                os.path.join(dirname, "PKG-INFO"), """
                    Metadata-Version: 1.0
                    Name: TestDevice
                    Version: 1.0
                    Summary: Some Test
                    Download-URL: https://example.com/whatever
                    Author-email: nobody@example.com
                    License: None
                """)
            with entry_points, pkg_info:
                self.run_async(self.dc, sleep(5))
                self.run_async(self.dc, instantiate("testServer",
                                                    "SomeDevice", "someName"))
        finally:
            os.rmdir(dirname)
