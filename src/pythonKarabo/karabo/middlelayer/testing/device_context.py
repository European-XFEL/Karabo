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
from asyncio import gather, sleep

from karabo.middlelayer.device_server import MiddleLayerDeviceServer

SHUTDOWN_TIME = 2


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
