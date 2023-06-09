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
import time
from asyncio import gather, get_event_loop, sleep
from contextlib import contextmanager
from datetime import datetime
from unittest import main, mock

from karabo.middlelayer import (
    Device, DeviceClientBase, Hash, KaraboError, Schema,
    getConfigurationFromPast, getInstanceInfo, getSchemaFromPast, getTopology,
    slot)
from karabo.middlelayer_api.synchronization import background, synchronize
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst

DEVICE_ID = "data_logger_device_id"

conf = {
    "classId": "DataLogReader",
    "_deviceId_": DEVICE_ID,
}


@synchronize
def _getLogReaderId(device):
    return DEVICE_ID


class HiddenDeviceClient(DeviceClientBase):
    def _initInfo(self):
        """Hide this device in the `client` section of the topology

        DeviceClientBase is a device, and this will influence the
        topology that the test `TestDeviceClientBase` expects
        in a non-trivial and time-dependent way.
        """
        info = super()._initInfo()
        info["type"] = "client"
        return info


class DataLogReader(Device):

    def _initInfo(self):
        info = super()._initInfo()
        info["TestInfo"] = "This is a karabo test info"
        return info

    @slot
    async def slotGetConfigurationFromPast(self, deviceId, timepoint):
        if deviceId == "aDeviceNotInHistory":
            raise KaraboError
        h = Hash('value', 42)
        s = Schema(name=DEVICE_ID, hash=Hash())

        return h, s, True, datetime.now().isoformat()


class TestDeviceClient(DeviceTest):

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = DataLogReader(conf)
        with cls.deviceManager(lead=cls.dev):
            yield

    @async_tst
    async def test_getConfigurationSchemaFromPast(self):
        with mock.patch('karabo.middlelayer_api.device_client._getLogReaderId',
                        new=_getLogReaderId):

            time = datetime.now().isoformat()

            with self.assertRaises(KaraboError):
                await getConfigurationFromPast("aDeviceNotInHistory", time)

            h = await getConfigurationFromPast("aDeviceInHistory", time)
            self.assertIsInstance(h, Hash)
            self.assertEqual(h['value'], 42)

            s = await getSchemaFromPast("aDeviceInHistory", time)
            self.assertIsInstance(s, Schema)
            self.assertEqual(s.name, DEVICE_ID)

    @async_tst
    async def test_getInstanceInfo(self):
        info = await getInstanceInfo(DEVICE_ID)
        self.assertEqual(info["TestInfo"], "This is a karabo test info")
        self.assertEqual(info["classId"], "DataLogReader")


class TestDeviceClientBase(DeviceTest):

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = HiddenDeviceClient(conf)
        with cls.deviceManager(lead=cls.dev):
            yield

    @async_tst
    async def test_system_topology(self):
        """This will test the thread access to the topology Hash"""
        class DefaultDevice(Device):
            __version__ = "1.2.3"

        self.assertEqual(len(getTopology()["device"]), 0,
                         f"Topology not empty: {getTopology()['device']}")

        devices = []

        NUMBER_DEVICES = 20
        for number in range(NUMBER_DEVICES):
            dev = DefaultDevice(
                dict(_deviceId_=f"other{number}", _serverId_="tserver"))
            dev.startInstance()
            devices.append(dev)

        finished = False

        def check_increasing():
            nonlocal finished
            count = 0
            while True:
                num_devices = len(getTopology()["device"])
                count += 1
                if num_devices == NUMBER_DEVICES:
                    # release test
                    finished = True
                    break
                elif count > 25:
                    # Max 25 seconds until a device appears
                    finished = True
                    break
                elif num_devices > 0:
                    # Devices appear in topology, start
                    # checking topology from this thread
                    for _ in range(100):
                        getTopology()
                        time.sleep(0.01)
                else:
                    time.sleep(1)

        # Start a thread in the background
        loop = get_event_loop()
        background(loop.run_coroutine_or_thread(check_increasing))

        # Busy wait until the thread is finished
        while not finished:
            await sleep(0.5)
        self.assertGreaterEqual(len(getTopology()["device"]),
                                NUMBER_DEVICES)

        # Kill all devices
        futures = [d.slotKillDevice() for d in devices]
        await gather(*futures, return_exceptions=False)


if __name__ == "__main__":
    main()
