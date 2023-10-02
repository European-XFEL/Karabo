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
from asyncio import set_event_loop, sleep

import pytest

from .eventloop import EventLoop
from .signalslot import SignalSlotable

SHUTDOWN_TIME = 2


@pytest.fixture(scope="module")
def event_loop():
    """This is the eventloop fixture for pytest asyncio

    It automatically comes with a broker connection via
    a signal slotable
    """

    loop = EventLoop()
    set_event_loop(loop)
    try:
        lead = SignalSlotable(
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
