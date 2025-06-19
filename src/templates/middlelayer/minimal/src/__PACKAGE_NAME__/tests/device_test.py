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

from karabo.middlelayer.testing import AsyncDeviceContext

from ..__CLASS_NAME__ import __CLASS_NAME__


_DEVICE_ID = "Test__CLASS_NAME__"
_DEVICE_CONFIG = {
    "deviceId": _DEVICE_ID,
}


@pytest.mark.timeout(30)
@pytest.mark.asyncio(loop_scope="module")
async def test_device():
    device = __CLASS_NAME__(_DEVICE_CONFIG)
    async with AsyncDeviceContext(device=device) as ctx:
        assert ctx.instances["device"] is device
        assert ctx.instances["device"].deviceId == _DEVICE_ID
