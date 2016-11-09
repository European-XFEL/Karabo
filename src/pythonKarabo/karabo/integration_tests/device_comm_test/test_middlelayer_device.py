from asyncio import async, coroutine
from contextlib import closing
import gc
import os
import os.path
from unittest import TestCase
import weakref

from karabo.middlelayer import getClasses, getDevice, getDevices, getServers, instantiate, Macro, Hash, shutdown, sleep, Slot
from karabo.middlelayer_api.cli import DeviceClient
from karabo.middlelayer_api.device_server import DeviceServer
from karabo.middlelayer_api.tests.eventloop import setEventLoop


class Remote(Macro):
    @Slot()
    def instantiate(self):
        conf = Hash(
            "count", Hash(),
            "visibility", 0,
            "log", Hash(),
            "Logger", Hash(
                "categories", [
                    Hash("Category", Hash(
                        "name", "karabo", "additivity", False,
                        "appenders", [
                            Hash("RollingFile", Hash(
                                "layout", Hash("Pattern", Hash(
                                    "format", "something")),
                                "filename", "some.log"))]))],
                "appenders", [
                    Hash("Ostream", Hash(
                        "layout", Hash(
                            "Pattern", Hash("format", "some format")))),
                    Hash("RollingFile", Hash(
                        "layout", Hash("Pattern", Hash(
                            "format", "other format")),
                        "filename", "other.log")),
                    Hash("Network", Hash())]))
        instantiate("testServer", "MiddleLayerTestDevice", "other",
                    configuration=conf, do_count=Hash())

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

    def tearDown(self):
        try:
            os.remove("serverId.xml")
        except FileNotFoundError:
            pass
        os.chdir(self.__starting_dir)

    @coroutine
    def init_server(self, server):
        """initialize the device server and a device on it

        the device on the device server is started from within a
        macro `remote`, such that we can test device instantiation
        from macros."""
        yield from sleep(0.3)
        remote = Remote(_deviceId_="remote")
        yield from remote.startInstance()
        proxy = yield from getDevice("remote")
        yield from proxy.instantiate()
        yield from sleep(1.1)
        return (yield from getDevice("other"))

    @coroutine
    def shutdown_server(self):
        """shutdown the device started by `init_server`

        Again a macro is called to shut down the device on the server,
        to test device shutdown in macros"""
        proxy = yield from getDevice("remote")
        yield from proxy.shutdown()
        yield from sleep(0.02)

    @coroutine
    def check_server_topology(self, dc):
        schema, classId, serverId = yield from dc.call(
            "testServer", "slotGetClassSchema", "MiddleLayerTestDevice")
        self.assertEqual(serverId, "testServer")
        self.assertEqual(classId, "MiddleLayerTestDevice")
        self.assertEqual(schema.name, "MiddleLayerTestDevice")
        h = schema.hash
        self.assertEqual(h["counter", ...], {
            'accessMode': 4,
            'assignment': 0,
            'defaultValue': -1,
            'metricPrefixSymbol': '',
            'nodeType': 0,
            'requiredAccessLevel': 0,
            'unitSymbol': 'N_A',
            'valueType': 'INT32'})

        self.assertIn("testServer", getServers())
        self.assertIn("other", getDevices("testServer"))
        self.assertIn("MiddleLayerTestDevice", getClasses("testServer"))
        self.assertNotIn("SomeBlupp", getClasses("testServer"))

    def test_server(self):
        """test the full lifetime of a python device server

        this test
          * starts a device client
          * starts a device server
          * starts a macro which tells said device server to start a device
          * checks that the other device shows up in the topology
          * let the macro tell to shut down that device
          * checks everything is cleaned up afterwards
          * kills the device server
        """
        loop = setEventLoop()

        with closing(loop):
            dc = DeviceClient(dict(_deviceId_="dc"))
            dc.startInstance()

            server = DeviceServer(
                dict(serverId="testServer",
                     pluginDirectory=os.path.dirname(__file__)))
            server.startInstance()
            proxy = loop.run_until_complete(
                loop.create_task(self.init_server(server), server))
            # test that onInitialization was run properly
            self.assertEqual(proxy.something, 222)
            loop.run_until_complete(
                loop.create_task(self.check_server_topology(dc), dc))
            self.assertIn("other", server.deviceInstanceMap)
            r = weakref.ref(server.deviceInstanceMap["other"])
            with proxy:
                loop.run_until_complete(
                    loop.create_task(self.shutdown_server(), server))
                self.assertNotIn("other", server.deviceInstanceMap)
                gc.collect()
                self.assertIsNone(r())
                async(dc.slotKillDevice())
                loop.run_until_complete(server.slotKillServer())
                self.assertEqual(proxy.something, 111)
            loop.run_forever()
