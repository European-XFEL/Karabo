from asyncio import sleep
from contextlib import contextmanager
import os
import string
from random import choice
import time
import unittest
import uuid

from karabo.middlelayer_api.tests.eventloop import async_tst, DeviceTest

from karabo.influxdb import get_line_fromdicts, InfluxDbClient, Results
from ..dlraw2influx import DlRaw2Influx
from ..dlschema2influx import DlSchema2Influx

_host = os.environ.get("KARABO_TEST_INFLUXDB_HOST", "localhost")
_port = int(os.environ.get("KARABO_TEST_INFLUXDB_PORT", "8086"))
_topic = f"test_{uuid.uuid4()}"
_url = f"http://{_host}:{_port}/query"
_fake_device = 'DOMAIN/MOTOR/BOB'
_fake_key = 'position'


class Influx_TestCase(DeviceTest):
    points = 600

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.client = InfluxDbClient(_host, _port)
        cls.loop.run_until_complete(cls.inject())
        cls.lead = cls.client
        yield
        cls.loop.run_until_complete(cls.client.drop_db(_topic))
        cls.client.disconnect()

    @classmethod
    async def inject(cls):
        """Injects historical data
        
        from 0.2 * cls.point seconds to 0.1 * cls.point second (excluded), 
        one value every 0.1 s in the value-FLOAT field
        from 0.1 * cls.point seconds to 0 (excluded), 
        one value every 0.1 s in the value-DOUBLE field
        """
        await cls.client.connect()
        await cls.client.create_db(_topic)
        cls.client.db = _topic
        cls.step = 1e5
        timestamp = float(time.time()) * 1e6
        last_tid = 10 * cls.points
        seq = [(last_tid - i, int(timestamp - i * cls.step), 22. + i)
               for i in range(2 * cls.points, cls.points, -1)]
        cls.begin = seq[0][1]
        data = [get_line_fromdicts(
                    _fake_device,
                    {f'{_fake_key}-FLOAT': value, '_tid': tid},
                    tag_dict={"karabo_user": "."},
                    timestamp=ts)
                for tid, ts, value in seq]
        seq = [(last_tid - i, int(timestamp - i * cls.step), 22. + i)
               for i in range(cls.points, 0, -1)]
        data.extend([get_line_fromdicts(
                         _fake_device,
                         {f'{_fake_key}-DOUBLE': value, '_tid': tid},
                         tag_dict={"karabo_user": "."},
                         timestamp=ts)
                     for tid, ts, value in seq])
        cls.end = seq[-1][1]
        r = await cls.client.write(b"\n".join(data))
        assert r.code == 204
        cls.init_ts = timestamp

    @async_tst
    async def test_init(self):
        dbs = await self.client.get_dbs()
        assert _topic in dbs

    @async_tst
    async def test_write_large(self):
        letters = string.ascii_lowercase
        def make_random():
            return ''.join(
                choice(letters)
                for _ in range(int(1e6)))
        big_string = make_random()
        line = get_line_fromdicts(
            _fake_device, {'stringvalue': big_string})
        r = await self.client.write(line)
        self.assertEqual(r.code, 204)
        # Influx will need to store and fetch since this is big

        attempts = 5
        r = []
        while attempts > 0:
            try:
                r, cols = await self.client.get_last_value(
                    _fake_device, "stringvalue")
                break
            except AssertionError:
                await sleep(0.2)
                attempts -= 1
        assert attempts > 0, "timeout on get_last_value"
        ts, last = next(r)
        self.assertEqual(last, big_string)
        ts = time.time() * 1e6 - ts
        self.assertTrue(ts > 0, 'timestamp is olds')

    @async_tst
    async def test_inject_olddata(self):
        timestamp = float(time.time()) * 1e6
        seq = [(int(timestamp - i * self.step), 22. + i)
               for i in range(self.points, 0, -1)]
        data = [get_line_fromdicts(
                _fake_device, {'value': value}, timestamp=ts)
                for ts, value in seq]
        r = await self.client.write(b"\n".join(data))
        self.assertEqual(r.code, 204)
        r, cols = await self.client.get_field_values(
            _fake_device, 'value', seq[0][0], seq[-1][0])
        for i, tv in enumerate(r):
            timestamp, value = tv
            self.assertEqual(seq[i][0], timestamp)
            self.assertEqual(seq[i][1], value)
        # now get a sub-sample
        k = 10
        begin = seq[0][0]
        end = seq[-1][0]
        length = self.points // k

        r, cols = await self.client.get_field_values_samples(
            _fake_device, 'value', begin, end, length)
        self.assertEqual(len(cols), 2)
        # note: this is to remove a test fragility
        # there are a good deal of int to float to int conversion
        self.assertIn(len(list(r))-length, (-1, 0))

    @async_tst
    async def test_write_get_last(self):
        timestamp = int((time.time() + 1) * 1e6)

        r = await self.client.write_measurement(
            _fake_device, dict(value=24., BOBBY=25.), timestamp=timestamp)
        self.assertEqual(r.code, 204)
        attempts = 5
        while attempts > 0:
            try:
                r, cols = await self.client.get_last_value(_fake_device, "value")
                break
            except RuntimeError as e:
                print(e)
                await sleep(0.1)
                attempts -= 1
        self.assertTrue(
            attempts > 0,
            "'get_last_value' failed 5 consecutive attempts")
        
        ts, last = next(r)
        self.assertEqual(len(cols), 2)
        self.assertEqual(ts, timestamp)
        self.assertEqual(last, 24.)

    @async_tst
    async def test_query_karabodata(self):
        r, cols = await self.client.get_field_values(
            _fake_device, f"{_fake_key}-.*", self.begin, self.end)
        r = list(r)
        self.assertEqual(len(r), 2 * self.points)

        for i, vals in enumerate(r):
            self.assertEqual(len(vals), len(cols))
            if i < self.points:
                self.assertIsNone(vals[1]) # first half has f"{_fake_key}-FLOAT"
            else:
                self.assertIsNone(vals[2]) # second half has f"{_fake_key}-DOUBLE"
        # because of the silly quotes, `time` is last
        expected_keys = set(('time', f'"{_fake_key}-FLOAT"', f'"{_fake_key}-DOUBLE"'))

        self.assertEqual(set(cols), expected_keys)
        r = await self.client.get_field_count(
            _fake_device, f"{_fake_key}-DOUBLE", self.begin, self.end)
        self.assertEqual(r[f'"{_fake_key}-DOUBLE"'], self.points)
        r = await self.client.get_field_count(
            _fake_device, f"{_fake_key}-FLOAT", self.begin, self.end)
        self.assertEqual(r[f'"{_fake_key}-FLOAT"'], self.points)
        r = await self.client.get_field_count(
            _fake_device, f'{_fake_key}-.*', self.begin, self.end)
        expected_keys.remove('time')
        self.assertEqual(expected_keys, set(r.keys()))
        for count in r.values():
            self.assertEqual(count, self.points)

        k = 10
        length = 15
        r, cols = await self.client.get_field_values_samples(
            _fake_device, [f"{_fake_key}-DOUBLE", "_tid"],
            self.begin + self.points * self.step, self.end, length)
        r = list(r)
        self.assertEqual(len(cols), 3)

        for col in range(1, 3):
            # the values for a SAMPLE are sparse.
            not_null_val = [val[col] for val in r if val[col] is not None]
            self.assertEqual(length, len(not_null_val))

        interval = (self.step * self.points // k)
        r, cols = await self.client.get_field_values_group(
            _fake_device, [f"{_fake_key}-.*", "_tid"],
            self.begin + self.points * self.step, self.end, interval)
        # the result of grouped points might be either k + 1 and k
        # there are rounding error occuring on the DB side for intervals
        self.assertIn(len(list(r)) - k, (1, 0))

    @async_tst
    async def test_has_tag(self):
        has = await self.client.field_has(
            _fake_device, f"{_fake_key}-DOUBLE",
            '"\\"karabo_user\\"" = \'"."\'')
        self.assertTrue(has)
        has = await self.client.field_has(
            _fake_device, f"{_fake_key}-DOUBLE",
            '"\\"karabo_user\\"" = \'"Charlemagne"\'')
        self.assertFalse(has)

    @async_tst
    async def test_migration(self):
        path = os.path.join(
            os.path.dirname(__file__), 'sample_data', 'archive_0.txt')
        conf2db = DlRaw2Influx(path=path, topic=_topic, host=_host,
            port=_port, device_id="cppserver_3_DataGenerator_1", dry_run=False)
        await conf2db.run()
        path = os.path.join(
            os.path.dirname(__file__), 'sample_data', 'archive_schema.txt')
        schema2db = DlSchema2Influx(path=path, topic=_topic, host=_host,
            port=_port, device_id="cppserver_3_DataGenerator_1", dry_run=False)
        await schema2db.run()


if __name__ == '__main__':
    unittest.main()
