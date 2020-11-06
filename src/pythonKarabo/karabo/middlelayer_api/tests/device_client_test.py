from contextlib import contextmanager
from datetime import datetime
from unittest import mock

from karabo.middlelayer_api.tests.eventloop import async_tst, DeviceTest
from karabo.middlelayer import (
    coslot, Device, getConfigurationFromPast, getSchemaFromPast, Schema,
    Hash, KaraboError)
from karabo.middlelayer_api.synchronization import synchronize


DEVICE_ID = "data_logger_device_id"

conf = {
    "classId": "DataLogReader",
    "_deviceId_": DEVICE_ID,
}


@synchronize
def _getLogReaderId(device):
    return DEVICE_ID


class DataLogReader(Device):

    @coslot
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
