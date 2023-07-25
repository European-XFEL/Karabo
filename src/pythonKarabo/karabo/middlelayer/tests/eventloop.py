# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import uuid
import weakref
from asyncio import gather, get_event_loop, set_event_loop, sleep, wait_for
from contextlib import ExitStack, contextmanager
from functools import partial, wraps
from unittest import TestCase
from unittest.mock import Mock

import pytest

from karabo.middlelayer.device_server import MiddleLayerDeviceServer
from karabo.middlelayer.eventloop import EventLoop, ensure_coroutine
from karabo.middlelayer.signalslot import SignalSlotable
from karabo.middlelayer.synchronization import synchronize_notimeout

SHUTDOWN_TIME = 2


def create_instanceId(name=None):
    """Create a unique instanceId with `name` and append a uuid"""
    if name is None:
        name = "test-mdl"
    return f"{name}-{uuid.uuid4()}"


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
    timeout = 20

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
            cls.loop.close()
            set_event_loop(cls.old_event_loop)

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        """This context manager is run around the test class"""
        cls.lead = Mock()
        cls.lead.deviceId = f"test-mdl-{uuid.uuid4()}"
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
        total_time = cls.timeout
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
    """This is the eventloop fixture for pytest asyncio

    It automatically comes with a broker connection via
    a signal slotable
    """
    loop = setEventLoop()
    try:
        lead = SignalSlotable(
            {"_deviceId_": create_instanceId()})
        loop.run_until_complete(lead.startInstance())
        instance_handler = loop.instance

        def instance(loop):
            """A new instance handler for the loop"""
            return instance_handler() or lead

        loop.instance = instance.__get__(loop, type(loop))
        yield loop
    finally:
        loop.run_until_complete(lead.slotKillDevice())
        loop.run_until_complete(sleep(SHUTDOWN_TIME))
        loop.close()


@contextmanager
def switch_instance(instance):
    """Switch the owning instance of the loop"""
    loop = get_event_loop()
    try:
        loop_instance = loop.instance
        loop.instance = weakref.ref(instance)
        yield
    finally:
        loop.instance = loop_instance


def create_device_server(serverId, plugins=[], config={}):
    """Create a device server instance with `plugins`

    :param serverId: the serverId of the server
    :param config: The server configuration
    :param plugins: Optional plugins list of device classes
    """
    config.update({"serverId": serverId,
                   "heartbeatInterval": 20})
    server = MiddleLayerDeviceServer(config)
    if plugins:

        async def scanPluginsOnce(server):
            return False

        server.scanPluginsOnce = scanPluginsOnce.__get__(
            server, type(server))
        server.plugins = {klass.__name__: klass for klass in plugins}
    return server


class AsyncDeviceContext:
    """This class is responsible to instantiate and shutdown device classes

    :param timeout: The timeout in seconds to wait for instantiation of an
                    instance.
    """

    def __init__(self, timeout=20, **instances):
        assert "sigslot" not in instances, "sigslot not allowed"
        self.instances = instances
        self.timeout = timeout

    async def wait_online(self, instances):
        # Make sure that we definitely release here with a total
        # sleep time. All times in seconds
        sleep_time = 0.1
        total_time = self.timeout
        while total_time >= 0:
            onlines = [d.is_initialized for d in instances.values()]
            if all(onlines):
                break
            total_time -= sleep_time
            await sleep(sleep_time)

    async def __aenter__(self):
        if self.instances:
            await gather(*(d.startInstance() for d in self.instances.values()))
            await self.wait_online(self.instances)

        return self

    async def __aexit__(self, exc_type, exc, exc_tb):
        await self.shutdown()
        # Shutdown time
        await sleep(SHUTDOWN_TIME)

    async def device_context(self, **instances):
        """Relay control of device `instances`

        The instances are added to the device dictionary of the class
        and shutdown in the finish.
        """
        devices = {}
        for k, v in instances.items():
            assert k not in self.instances
            devices.update({k: v})
        self.instances.update(devices)
        await gather(*(d.startInstance() for d in devices.values()))
        await self.wait_online(devices)

    async def shutdown(self):
        devices = [d for d in self.instances.values()
                   if not isinstance(d, MiddleLayerDeviceServer)]
        if devices:
            await gather(*(d.slotKillDevice() for d in devices))

        servers = [s for s in self.instances.values()
                   if isinstance(s, MiddleLayerDeviceServer)]
        if servers:
            await gather(*(s.slotKillDevice() for s in servers))
        self.instances = {}

    def __getitem__(self, instance):
        """Convenience method to get an instance from the context"""
        return self.instances[instance]


@synchronize_notimeout
async def sleepUntil(condition, timeout=None):
    """Sleep until some condition is valid

    :param timeout: The timeout parameter, defaults to `None`. (no timeout)
    """

    async def internal_wait():
        while not condition():
            await sleep(0.05)

    return await wait_for(internal_wait(), timeout=timeout)
