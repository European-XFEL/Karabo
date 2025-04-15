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
from asyncio import gather, set_event_loop, sleep, wait_for
from contextlib import ExitStack, contextmanager
from functools import partial, wraps
from unittest import TestCase
from unittest.mock import Mock

from karabo.middlelayer.device_server import MiddleLayerDeviceServer
from karabo.middlelayer.eventloop import EventLoop
from karabo.middlelayer.utils import ensure_coroutine


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
            set_event_loop(cls.loop)
            cls.exit_stack.enter_context(cls.lifetimeManager())
            cls.exit_stack = cls.exit_stack.pop_all()

    @classmethod
    def tearDownClass(cls):
        try:
            with cls.exit_stack:
                pass
        finally:
            cls.loop.close()
            set_event_loop(None)

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        """This context manager is run around the test class"""
        cls.lead = Mock()
        cls.lead.deviceId = f"test-mdl-{uuid.uuid4()}"
        cls.lead._sigslot = Mock()
        cls.lead._sigslot.loop = cls.loop
        cls.lead._sigslot.tasks = set()
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
