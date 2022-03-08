import uuid
from asyncio import gather, get_event_loop, set_event_loop, sleep, wait_for
from contextlib import ExitStack, contextmanager
from functools import partial, wraps
from unittest import TestCase
from unittest.mock import Mock

import pytest

from karabo.middlelayer_api.device_server import MiddleLayerDeviceServer
from karabo.middlelayer_api.eventloop import EventLoop, ensure_coroutine
from karabo.middlelayer_api.signalslot import SignalSlotable


def async_tst(f=None, *, timeout=None):
    if f is None:
        assert timeout is not None, 'timeout must be given!'
        return partial(async_tst, timeout=timeout)

    f = ensure_coroutine(f)
    return sync_tst(f, timeout=timeout)


def sync_tst(f=None, *, timeout=None):
    if f is None:
        assert timeout is not None, 'timeout must be given!'
        return partial(sync_tst, timeout=timeout)

    # Default timeout is 30 seconds. Slower tests can give longer values
    timeout = 30 if timeout is None else timeout

    @wraps(f)
    def wrapper(self):
        task = self.loop.create_task(
            self.loop.run_coroutine_or_thread(f, self), self.lead)
        self.loop.run_until_complete(wait_for(task, timeout))
    return wrapper


class DeviceTest(TestCase):

    @classmethod
    def setUpClass(cls):
        with ExitStack() as cls.exit_stack:
            cls.loop = EventLoop()
            cls.old_event_loop = get_event_loop()
            set_event_loop(cls.loop)
            cls.exit_stack.enter_context(cls.lifetimeManager())
            cls.exit_stack = cls.exit_stack.pop_all()

    @classmethod
    def tearDownClass(cls):
        try:
            with cls.exit_stack:
                pass
        finally:
            # return the event loop at the conditions we found it.
            # multiple tests might fail if we do not do this.
            # also: closing the event loop in a `finally`
            # as insurance against failures in exit_stack
            cls.loop.stop()
            cls.loop.close()
            set_event_loop(cls.old_event_loop)

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        """This context manager is run around the test class"""
        cls.lead = Mock()
        cls.lead.deviceId = "test-mdl-{}".format(uuid.uuid4())
        cls.lead._ss = Mock()
        cls.lead._ss.loop = cls.loop
        cls.lead._ss.tasks = set()
        yield

    @classmethod
    @contextmanager
    def deviceManager(cls, *devices, lead):
        """Manage the devices to run during the tests

        `devices` are the devices that should be run during the tests,
        and `lead` is the device under which name the tests are running.
        """
        if lead not in devices:
            devices += (lead,)
        cls.loop.run_until_complete(
            gather(*(d.startInstance() for d in devices)))
        cls.lead = lead

        # Make sure that we definately release here with a total
        # sleep time. All times in seconds
        sleep_time = 0.1
        total_time = 20
        while total_time >= 0:
            onlines = [d.is_initialized for d in devices]
            if all(onlines):
                break
            total_time -= sleep_time
            cls.loop.run_until_complete(sleep(sleep_time))
        yield
        cls.loop.run_until_complete(
            gather(*(d.slotKillDevice() for d in devices)))
        del cls.lead


def setEventLoop():
    loop = EventLoop()
    set_event_loop(loop)
    return loop


@pytest.fixture(scope="module")
def event_loop():
    """This is the eventloop fixture for pytest asyncio"""
    loop = EventLoop()
    yield loop
    loop.close()


def create_device_server(serverId, plugins=[]):
    """Create a device server instance with `plugins`

    :param serverId: the serverId of the server
    :param plugins: the plugins list of device classes
    """
    server = MiddleLayerDeviceServer({"serverId": serverId,
                                      "heartbeatInterval": 20})

    async def scanPluginsOnce():
        return False

    server.scanPluginsOnce = scanPluginsOnce
    server.plugins = {klass.__name__: klass for klass in plugins}
    return server


class AsyncDeviceContext:
    """This class is responsible to instantiate and shutdown device classes"""

    def __init__(self, sigslot=True, **instances):
        self.servers = {k: v for k, v in instances.items()
                        if isinstance(v, MiddleLayerDeviceServer)}
        self.devices = {k: v for k, v in instances.items()
                        if not isinstance(v, MiddleLayerDeviceServer)}
        self.sigslot = sigslot
        self.instance = None

    async def wait_online(self, instances):
        # Make sure that we definitely release here with a total
        # sleep time. All times in seconds
        sleep_time = 0.1
        total_time = 20
        while total_time >= 0:
            onlines = [d.is_initialized for d in instances.values()]
            if all(onlines):
                break
            total_time -= sleep_time
            await sleep(sleep_time)

    async def __aenter__(self):
        loop = get_event_loop()
        if self.sigslot:
            instance = SignalSlotable(
                {"_deviceId_": f"test-context-mdl-{uuid.uuid4()}"})
            await instance.startInstance()
            loop.instance = lambda: instance
            self.instance = instance
        if self.servers:
            await gather(*(d.startInstance() for d in self.servers.values()))
            await self.wait_online(self.servers)
        if self.devices:
            await gather(*(d.startInstance() for d in self.devices.values()))
            await self.wait_online(self.devices)
        return self

    async def __aexit__(self, exc_type, exc, exc_tb):
        if self.devices:
            await gather(*(d.slotKillDevice() for d in self.devices.values()))
        if self.servers:
            await gather(*(d.slotKillDevice() for d in self.servers.values()))
        if self.instance is not None:
            await self.instance.slotKillDevice()
            del self.instance

    async def device_context(self, **instances):
        """Relay control of device `instances`

        The instances are added to the device dictionary of the class
        and shutdown in the finish.
        """
        devices = {}
        for k, v in instances.items():
            assert k not in self.devices
            devices.update({k: v})
        self.devices.update(devices)
        await gather(*(d.startInstance() for d in devices.values()))
        await self.wait_online(devices)
