from asyncio import get_event_loop
import os
import time
import uuid
import unittest
from contextlib import contextmanager
from datetime import datetime


from karabo.influxdb import (
    get_line_fromdicts, InfluxDbClient, lines_fromhash)
from karabo.middlelayer import Hash, Timestamp
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst
from karabo.middlelayer_devices.influxdb_reader import DataLogReader
from karabo.middlelayer_devices.influxdb_logger import InfluxDBLogger

_host = os.environ.get("KARABO_TEST_INFLUXDB_HOST", "localhost")
_port = int(os.environ.get("KARABO_TEST_INFLUXDB_PORT", "8086"))
_user = os.environ.get("KARABO_TEST_INFLUXDB_USER", "infusr")
_password = os.environ.get("KARABO_TEST_INFLUXDB_USER_PASSWORD", "usrpwd")
_adm_user = os.environ.get("KARABO_TEST_INFLUXDB_ADMUSER", "infadm")
_adm_user_password = os.environ.get("KARABO_TEST_INFLUXDB_ADMUSER_PASSWORD",
                                    "admpwd")
_topic = os.environ.get("KARABO_TEST_INFLUXDB_DB", f"test_{uuid.uuid4()}")
_fake_device = 'DOMAIN/MOTOR/BOB'
_fake_key = 'position'


class TestInfluxDbReader(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.loop.run_until_complete(cls.inject())
        cls.reader = DataLogReader({"_deviceId_": "reader",
                                    "host": _host,
                                    "port": _port,
                                    "user": _adm_user,
                                    "password": _adm_user_password})
        with cls.deviceManager(lead=cls.reader):
            yield
        cls.loop.run_until_complete(cls.clean_up())

    @classmethod
    async def clean_up(cls):
        await cls.client.drop_measurement(_fake_device)
        cls.client.disconnect()

    @classmethod
    async def inject(cls):
        _topic = get_event_loop().topic
        cls.points = 600
        cls.client = InfluxDbClient(_host, _port,
                                    user=_adm_user,
                                    password=_adm_user_password)
        await cls.client.connect()
        # Tries to create the database if it doesn't exist.
        dbs = await cls.client.get_dbs()
        if _topic not in dbs:
            await cls.client.create_db(_topic)
        cls.client.db = _topic
        step = 1e5
        timestamp = float(time.time()) * 1e6
        last_tid = 10 * cls.points
        seq = [(last_tid - i, int(timestamp - i * step), 22. + i)
               for i in range(2 * cls.points, cls.points, -1)]
        cls.begin = seq[0][1]
        data = [get_line_fromdicts(
                    _fake_device,
                    {f'{_fake_key}-INT32': int(value), '_tid': tid},
                    tag_dict={'karabo_user': "."},
                    timestamp=ts)
                for tid, ts, value in seq]

        seq = [(last_tid - i, int(timestamp - i * step), 22. + i)
               for i in range(cls.points, 0, -1)]
        data.extend([get_line_fromdicts(
                         _fake_device,
                         {f'{_fake_key}-DOUBLE': float(value),
                          '_tid': tid},
                         tag_dict={'karabo_user': "."},
                         timestamp=ts)
                     for tid, ts, value in seq])
        cls.end = seq[-1][1]
        r = await cls.client.write(b"\n".join(data))
        assert r.code == 204, "Could not inject test data"
        cls.init_ts = timestamp

    @async_tst
    async def test_read(self):
        begin = datetime.utcfromtimestamp(self.begin / 1e6).isoformat()
        end = datetime.utcfromtimestamp(self.end / 1e6).isoformat()

        params = Hash("from", begin, "to", end, "maxNumData", 10 * self.points)
        deviceId, key, res = await self.reader.call(
            self.reader.deviceId, "slotGetPropertyHistory",
            _fake_device, _fake_key, params)
        self.assertEqual(deviceId, _fake_device)
        self.assertEqual(key, _fake_key)
        self.assertEqual(len(res), 2 * self.points)


if __name__ == '__main__':
    unittest.main()
