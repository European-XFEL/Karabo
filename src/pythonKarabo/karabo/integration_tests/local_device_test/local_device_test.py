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
import json

import pytest

from karabo.middlelayer import Hash, State, getProperties, instantiate
from karabo.middlelayer.testing import AsyncServerContext, assert_wait_property


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_local_device():
    serverId = "testLocalDevice"
    config = {"alice": {"classId": "WaiterDevice", "waiterId": "bob"}}
    init = json.dumps(config)
    server = AsyncServerContext(
        serverId, [f"init={init}",
                   "pluginNamespace=karabo.middlelayer_device_test"],
        verbose=True, api="middlelayer")
    async with server:
        # Alice is looking for bob
        await assert_wait_property(
            "alice", "state", State.CHANGING, timeout=20)
        props = await getProperties("alice", "initialDiscover")
        assert props["initialDiscover"] is False
        await instantiate(serverId, "WaiterDevice", "bob",
                          Hash("waiterId", "alice"))
        await assert_wait_property(
            "alice", "state", State.ON, timeout=20)
        props = await getProperties("alice", ["hasDevice", "hasCancellation"])
        assert props["hasDevice"] is True
        assert props["hasCancellation"] is True

        await assert_wait_property(
            "bob", "state", State.ON, timeout=10)
        # Bob could find alice immediately
        props = await getProperties("bob", "initialDiscover")
        assert props["initialDiscover"] is True
        props = await getProperties("bob", ["hasDevice", "hasCancellation"])
        assert props["hasDevice"] is True
        assert props["hasCancellation"] is False
