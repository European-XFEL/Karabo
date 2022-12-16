#############################################################################
# Author: __EMAIL__
#
# Created on __DATE__
# from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
#
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import pytest

from karabo.middlelayer.testing import AsyncDeviceContext, event_loop

from ..__CLASS_NAME__ import __CLASS_NAME__

_DEVICE_CONFIG = {
    "_deviceId_": "Test__CLASS_NAME__",
    "greeting": "buongiorno"
}


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_greeting(event_loop: event_loop):
    device = __CLASS_NAME__(_DEVICE_CONFIG)
    async with AsyncDeviceContext(device=device) as ctx:
        assert ctx.instances["device"] is device
        for greet in ("Buongiorno", "Guten Tag", "Moin Moin"):
            device.greeting = greet
            assert device.greeting.value == greet
            await device.hello()
            assert device.greeting.value == "Hello world!"
