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
import json

import pytest

from karabo.bound.testing import ServerContext, sleepUntil

from ..__CLASS_NAME__ import __CLASS_NAME__  # noqa

_DEVICE_ID = "TestDevice__CLASS_NAME__"
_DEVICE_CONFIG = {
    _DEVICE_ID: {"classId": "__CLASS_NAME__"},
}


@pytest.mark.timeout(30)
def test_device(eventLoop):
    init = json.dumps(_DEVICE_CONFIG)
    server = ServerContext("testServer__CLASS_NAME__",
                           ["log.level=DEBUG", f"init={init}"])
    with server:
        remote = server.remote()
        sleepUntil(lambda: _DEVICE_ID in remote.getDevices(), timeout=10)
        assert len(remote.getDevices()) > 0
