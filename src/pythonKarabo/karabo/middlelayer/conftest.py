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
from asyncio import DefaultEventLoopPolicy, set_event_loop, sleep

import pytest

from .device import DeviceClientBase
from .eventloop import EventLoop


def async_test_placeholder(func):
    return False


try:
    from pytest_asyncio import is_async_test
except Exception:
    is_async_test = async_test_placeholder


def async_test_placeholder(func):
    return False


try:
    from pytest_asyncio import is_async_test
except Exception:
    is_async_test = async_test_placeholder

SHUTDOWN_TIME = 2


class TopologyClient(DeviceClientBase):
    """A test client with SystemTopology information

    Note: The client does not wait by default on collecting
    the topology on startup.
    """
    wait_topology = False


@pytest.fixture(scope="module")
def event_loop():
    """This is the eventloop fixture for pytest asyncio

    It automatically comes with a broker connection via
    a signal slotable.
    """

    loop = EventLoop()
    set_event_loop(loop)
    try:
        lead = TopologyClient(
            {"_deviceId_": f"SigSlot-{uuid.uuid4()}"})
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


class KaraboTestLoopPolicy(DefaultEventLoopPolicy):
    def new_event_loop(self):
        loop = EventLoop()
        set_event_loop(loop)
        lead = TopologyClient(
            {"_deviceId_": f"topoClient-{uuid.uuid4()}"})
        loop.run_until_complete(lead.startInstance())
        instance_handler = loop.instance

        def instance(loop):
            """A new instance handler for the loop"""
            return instance_handler() or lead

        loop.lead = lead
        loop.instance = instance.__get__(loop, type(loop))

        close_handler = loop.close

        def new_close_handler(loop):
            loop.run_until_complete(lead.slotKillDevice())
            loop.run_until_complete(sleep(SHUTDOWN_TIME))
            return close_handler()

        loop.close = new_close_handler.__get__(loop, type(loop))

        return loop


@pytest.fixture(scope="module")
def event_loop_policy():
    return KaraboTestLoopPolicy()


def pytest_collection_modifyitems(items):
    pytest_asyncio_tests = (item for item in items if is_async_test(item))
    session_scope_marker = pytest.mark.asyncio(loop_scope="module")
    for async_test in pytest_asyncio_tests:
        async_test.add_marker(session_scope_marker, append=False)
