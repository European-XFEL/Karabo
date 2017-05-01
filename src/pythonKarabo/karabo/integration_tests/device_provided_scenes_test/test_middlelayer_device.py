from asyncio import async, coroutine
from contextlib import contextmanager
import gc
import os
import os.path
from textwrap import dedent
from unittest import TestCase
import weakref

from karabo.common.capabilities import Capabilities
from karabo.middlelayer import (
    connectDevice, Device, getClasses, getDevice, getDevices, getServers,
    instantiate, isAlive, KaraboError, Macro, shutdown, sleep, Slot)
from karabo.middlelayer_api.cli import DeviceClient
from karabo.middlelayer_api.device_server import DeviceServer
from karabo.middlelayer_api.tests.eventloop import setEventLoop


class Remote(Macro):
    @Slot()
    def instantiate(self):
        instantiate("testServerSceneProviders", "SceneProvidingMLDevice",
                    "SceneProvidingML")

        instantiate("testServerSceneProviders", "NonSceneProvidingMLDevice",
                    "NonSceneProvidingML")

    @Slot()
    def shutdown(self):
        shutdown("SceneProvidingML")
        shutdown("NonSceneProvidingML")


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
            dict(serverId="testServerSceneProviders", pluginDirectory=dirname))
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
        proxySp = yield from getDevice("SceneProvidingML")
        proxyNoSp = yield from getDevice("NonSceneProvidingML")
        return proxySp, proxyNoSp

    @coroutine
    def shutdown_server(self):
        """shutdown the device started by `init_server`

        Again a macro is called to shut down the device on the server,
        to test device shutdown in macros"""
        proxy = yield from getDevice("remote")
        yield from proxy.shutdown()
        yield from sleep(0.1)

    def test_sequential(self):
        """run multiple tests on the same server and devices

        this test
          * starts a device client
          * starts a device server
          * starts a macro which tells said device server to start a device
          * checks that the other device shows up in the topology
          * checks if instance info entries are correct for scenes available
          * kills the device server
        """
        proxySp, proxyNoSp = self.run_async(self.server, self.init_server())

        with proxySp, proxyNoSp:
            self.assertTrue(isAlive(proxySp))
            self.assertTrue(isAlive(proxyNoSp))

            with self.subTest(msg="Test 'scenesAvailable' in instance info"):
                topo = self.dc.systemTopology
                device = topo.get("device")

                self.assertTrue(device.hasAttribute("SceneProvidingML",
                                                    "capabilities"))

                capabilities = device.getAttribute("SceneProvidingML",
                                                   "capabilities")

                self.assertEqual(capabilities & Capabilities.PROVIDES_SCENES,
                                 1)

                self.assertTrue(device.hasAttribute("NonSceneProvidingML",
                                                    "capabilities"))

                capabilities = device.getAttribute("NonSceneProvidingML",
                                                   "capabilities")

                self.assertEqual(capabilities & Capabilities.PROVIDES_SCENES,
                                 0)

            self.loop.run_until_complete(self.server.slotKillServer())
        self.loop.run_forever()
