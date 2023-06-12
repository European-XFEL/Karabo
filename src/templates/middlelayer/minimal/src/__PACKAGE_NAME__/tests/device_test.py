#############################################################################
# Author: __EMAIL__
#
# Created on __DATE__
# from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
#
# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
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
