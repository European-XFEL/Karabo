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
import base64
import json
import os
import shutil
import string
import time
import unittest
from asyncio import sleep
from contextlib import contextmanager
from random import choice

from karabo.influx_db import InfluxDbClient, get_line_fromdicts
from karabo.influx_db.dl_migrator import DlMigrator
from karabo.middlelayer.tests.eventloop import DeviceTest, async_tst
from karabo.native import decodeBinary

_host = os.environ.get("KARABO_TEST_INFLUXDB_HOST", "localhost")
_port = int(os.environ.get("KARABO_TEST_INFLUXDB_PORT", "8086"))
_user = os.environ.get("KARABO_TEST_INFLUXDB_USER", "infusr")
_password = os.environ.get("KARABO_TEST_INFLUXDB_USER_PASSWORD", "usrpwd")
_adm_user = os.environ.get("KARABO_TEST_INFLUXDB_ADMUSER", "infadm")
_adm_user_password = os.environ.get("KARABO_TEST_INFLUXDB_ADMUSER_PASSWORD",
                                    "admpwd")
_topic = os.environ.get("KARABO_TEST_INFLUXDB_DB", "InfluxLogTest")
_fake_device = 'DOMAIN/MOTOR/BOB'
_fake_key = 'position'

MIGRATION_TEST_DEVICE = 'PropertyTestDevice'


class Influx_TestCase(DeviceTest):
    points = 600

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.adm_client = InfluxDbClient(_host, _port,
                                        user=_adm_user,
                                        password=_adm_user_password)
        cls.client = InfluxDbClient(_host, _port,
                                    user=_user, password=_password)
        cls.loop.run_until_complete(cls.inject())
        cls.lead = cls.client
        yield
        cls.loop.run_until_complete(
            cls.adm_client.drop_measurement(_fake_device))
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
        # Tries to create the database if it doesn't exist.
        dbs = await cls.adm_client.get_dbs()
        if _topic not in dbs:
            await cls.adm_client.create_db(_topic)
            await cls.adm_client.grant_all_to_user(_topic, _user)
        cls.client.db = _topic
        cls.adm_client.db = _topic
        await cls.adm_client.drop_measurement(_fake_device)
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
            _fake_device, dict(value=24., BOBBY=25.),
            timestamp=timestamp)
        self.assertEqual(r.code, 204)
        attempts = 5
        while attempts > 0:
            try:
                r, cols = await self.client.get_last_value(
                    _fake_device, "value")
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
                # first half has f"{_fake_key}-FLOAT"
                self.assertIsNone(vals[1])
            else:
                # second half has f"{_fake_key}-DOUBLE"
                self.assertIsNone(vals[2])
        # because of the silly quotes, `time` is last
        expected_keys = {
            'time', f'{_fake_key}-FLOAT', f'{_fake_key}-DOUBLE'}

        self.assertEqual(set(cols), expected_keys)
        r = await self.client.get_field_count(
            _fake_device, f"{_fake_key}-DOUBLE", self.begin, self.end)
        self.assertEqual(r[f'{_fake_key}-DOUBLE'], self.points)
        r = await self.client.get_field_count(
            _fake_device, f"{_fake_key}-FLOAT", self.begin, self.end)
        self.assertEqual(r[f'{_fake_key}-FLOAT'], self.points)
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
            'karabo_user = \'"."\'')
        self.assertTrue(has)
        has = await self.client.field_has(
            _fake_device, f"{_fake_key}-DOUBLE",
            'karabo_user = \'"Charlemagne"\'')
        self.assertFalse(has)

    @async_tst
    async def test_migration(self):

        # Creates temporary directories that will hold copies
        # of the sample data logging files to be migrated and
        # the output directory where the Migrator will save
        # fully processed files and partially processed files
        # along with the errors during their processing.
        #
        # The directory path with the copies of the sample data
        # logging files will follow the expected convention
        # of terminating in "raw" and have the deviceId in the
        # component right before "raw".
        file_dir = os.path.dirname(__file__)
        src_data_dir = os.path.join(file_dir,
                                    'sample_data',
                                    MIGRATION_TEST_DEVICE,
                                    'raw')
        output_dir = os.path.join(file_dir,
                                  'out_dir')
        tst_data_root = os.path.join(file_dir,
                                     'test_data')
        tst_data_dir = os.path.join(tst_data_root,
                                    MIGRATION_TEST_DEVICE,
                                    'raw')

        os.makedirs(tst_data_dir, exist_ok=True)
        os.makedirs(output_dir, exist_ok=True)

        shutil.copy(os.path.join(src_data_dir,
                                 'archive_4.txt'),
                    tst_data_dir)
        shutil.copy(os.path.join(src_data_dir,
                                 'archive_schema.txt'),
                    tst_data_dir)

        migrator = DlMigrator(
            db_name=_topic, input_dir=tst_data_root, output_dir=output_dir,
            write_url=f'http://{_host}:{_port}', write_user=_user,
            write_pwd=_password, read_url=f'http://{_host}:{_port}',
            read_user=_user, read_pwd=_password, lines_per_write=800,
            dry_run=False, write_timeout=20, concurrent_tasks=2)

        await migrator.run()

        # Checks the expected processed outputs - the 'archive_4.txt' and
        # 'archive_schema.txt' files are expected to be processed.
        # 'archive_4.txt' should produce warning files due to known issues
        # in its contents.
        # The other files support multiple migration runs with skipping of
        # files fully processed in previous runs.
        self.assertTrue(os.path.exists(os.path.join(output_dir,
                                                    'processed',
                                                    MIGRATION_TEST_DEVICE,
                                                    'archive_4.txt.ok')))
        self.assertTrue(os.path.exists(os.path.join(output_dir,
                                                    'part_processed',
                                                    MIGRATION_TEST_DEVICE,
                                                    'archive_4.txt.warn')))
        self.assertFalse(os.path.exists(os.path.join(output_dir,
                                                     'part_processed',
                                                     MIGRATION_TEST_DEVICE,
                                                     'archive_4.txt.err')))
        self.assertTrue(os.path.exists(os.path.join(output_dir,
                                                    'processed',
                                                    MIGRATION_TEST_DEVICE,
                                                    'archive_schema.txt.ok')))
        self.assertFalse(os.path.exists(
            os.path.join(output_dir, 'part_processed', MIGRATION_TEST_DEVICE,
                         'archive_schema.txt.err')))
        self.assertTrue(os.path.exists(os.path.join(output_dir,
                                                    '.previous_run.txt')))
        self.assertTrue(os.path.exists(os.path.join(output_dir,
                                                    '.processed_props.txt')))
        self.assertTrue(os.path.exists(os.path.join(output_dir,
                                                    '.processed_schemas.txt')))

        # Cleans up the directories with the copies of the sample
        # data logging files and the results of the migration job
        # that were created at the start of the test case.
        shutil.rmtree(tst_data_root)
        shutil.rmtree(output_dir)

        # Checks that the migrated data can be retrieved and has the
        # expected values - 2 rows for property table at a specific timestamp.
        attempts = 5
        while attempts > 0:
            try:
                r, cols = await self.client.get_last_value(
                    MIGRATION_TEST_DEVICE, "table-VECTOR_HASH"
                )
                break
            except RuntimeError as e:
                print(e)
                await sleep(0.1)
                attempts -= 1
        self.assertTrue(
            attempts > 0,
            "'get_last_value' failed 5 consecutive attempts")

        ts, table_enc = next(r)

        self.assertEqual(len(cols), 2)  # One for time and one for value
        import datetime

        # The timestamp returned from Influx is in microseconds.
        dt = datetime.datetime.fromtimestamp(
            ts/1_000_000, datetime.UTC
        )
        # The table property is a base64 encoding of a stream serialized by
        # Karabo's binary serializer.
        table = decodeBinary(base64.b64decode(table_enc))
        self.assertEqual(dt.day, 4)
        self.assertEqual(dt.month, 2)
        self.assertEqual(dt.year, 2020)
        self.assertEqual(dt.hour, 14)
        self.assertEqual(dt.minute, 32)
        self.assertEqual(dt.second, 57)

        self.assertEqual(len(table), 2)
        self.assertEqual(table[0]['e3'], 1188)
        self.assertEqual(table[1]['e3'], 4158)

        # check that nans are inserted as expected
        r, cols = await self.client.get_last_value(
            MIGRATION_TEST_DEVICE,
            "^floatProperty-FLOAT$")
        ts, value = next(r)
        self.assertEqual(value, 0)

        r, cols = await self.client.get_last_value(
            MIGRATION_TEST_DEVICE,
            "^floatProperty-FLOAT_INF$")
        ts, value = next(r)
        self.assertEqual(value, "nan")

        r, cols = await self.client.get_last_value(
            MIGRATION_TEST_DEVICE, "^vectors.stringProperty-.*|_tid")
        ts, *values = next(r)
        for col_idx, col_name in enumerate(cols[1:]):
            if col_name == "_tid":
                self.assertEqual(42000, values[col_idx])
            if col_name == "vectors.stringProperty-VECTOR_STRING":
                value = base64.b64decode(values[col_idx])
                self.assertEqual(
                    ["bla", "bli"], json.loads(value.decode('utf-8')))

        r, cols = await self.client.get_last_value(
            MIGRATION_TEST_DEVICE, "charProperty-CHAR")
        ts, value = next(r)
        self.assertEqual(b"A", base64.b64decode(value))

        r, cols = await self.client.get_last_value(
            MIGRATION_TEST_DEVICE, "vectors.charProperty-VECTOR_CHAR")
        ts, value = next(r)
        self.assertEqual(b"ABCDEF", base64.b64decode(value))
        properties_to_test = {
            'boolProperty': False,
            'int8Property': 33,
            'uint8Property': 177,
            'int16Property': 3_200,
            'uint16Property': 32_000,
            'int32Property': 99,
            'uint32Property': 32_000_000,
            'int64Property': 3_200_000_000,
            'uint64Property': 3200000000,
            'stringProperty': "Some arbitrary text.",
            'vectors.boolProperty': "1,0,1,0,1,0",
            'vectors.uint8Property': "41,42,43,44,45,46",  # noqa: this is decoded from base64
            'vectors.int8Property': "41,42,43,44,45,46",
            'vectors.int16Property': ",".join([str(20_041 + i)
                                               for i in range(6)]),
            'vectors.uint16Property': ",".join([str(10_041 + i)
                                                for i in range(6)]),
            'vectors.int32Property': ",".join([str(20_000_041 + i)
                                               for i in range(6)]),
            'vectors.uint32Property': ",".join([str(90_000_041 + i)
                                                for i in range(6)]),
            'vectors.int64Property': ",".join([str(20_000_000_041 + i)
                                               for i in range(6)]),
            'vectors.uint64Property': ",".join([str(90_000_000_041 + i)
                                                for i in range(6)]),
        }
        for key, expected in properties_to_test.items():
            r, cols = await self.client.get_last_value(
                MIGRATION_TEST_DEVICE,
                f"^{key}-.*")
            ts, value = next(r)
            self.assertEqual(expected, value, key)
        await self.adm_client.drop_measurement(MIGRATION_TEST_DEVICE)


if __name__ == '__main__':
    unittest.main()
