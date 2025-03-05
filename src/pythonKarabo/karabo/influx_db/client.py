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
import asyncio
import base64
import numbers
import re
import time
from json import loads
from textwrap import dedent
from urllib.parse import urlencode

from tornado.httpclient import AsyncHTTPClient, HTTPError

from karabo.native import (
    HASH_TYPE_TO_XML_TYPE, Hash, get_hash_type_from_data, string_from_hashtype)

from .dlutils import escape_tag_field_key


def get_key_value(key, value):
    """Returns a binary string representing a key-value pair

    the line protocol expects for tags and fields a key-value pair
    https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference/
    """
    if isinstance(value, str):
        return b''.join(
            [f'{key}=\"'.encode(),
             escape_tag_field_key(value).encode('utf-8'),
             b'\"'])
    elif isinstance(value, bool):
        if value:
            return f'{key}=t'.encode()
        else:
            return f'{key}=f'.encode()
    elif isinstance(value, numbers.Integral):
        return f'{key}={value}i'.encode()
    elif isinstance(value, numbers.Real):
        return f'{key}={repr(value)}'.encode()
    else:
        data = string_from_hashtype(value)
        return b''.join(
            [f'{key}=\"'.encode(),
                escape_tag_field_key(data).encode('utf-8'),
                b'\"'])


def get_line_fromdicts(measurement, field_dict, tag_dict=None, timestamp=None):
    """Returns a line protocol formatted line from its components
    """
    fields = b','.join([get_key_value(key, value)
                        for key, value in field_dict.items()])
    assert fields, 'fields are mandatory'

    if tag_dict is not None:
        tags = b''.join(
            (b',',
             b','.join([get_key_value(key, value)
                        for key, value in tag_dict.items()])
             )
        )
    else:
        tags = b''

    # timestamp precision is set in `write` is us
    timestamp = timestamp or int(1e6*time.time())
    timestamp = f' {timestamp}'.encode()
    return b''.join(
        (measurement.encode('utf-8'), tags, b' ', fields, timestamp))


def lines_fromhash(device_id, hash_, tags=""):
    """Returns a generator of bytes from a karabo Hash

    """
    timestamp = 0
    tid = 0
    fields = []
    yield device_id.encode('utf-8')
    yield tags.encode('utf-8')
    yield b' '

    # XXX: for the moment make only one line.
    # Ideally 1 line per timestamp should be influxed
    def _serialize(h, prefix):
        nonlocal timestamp, tid, fields
        for k, v, a in h.iterall():
            newts = int(a.get("frac", 0) // 10**12) +\
                int(a.get("sec", 0)) * 10**6
            newtid = a.get("tid", 0)
            if newts > timestamp:
                timestamp = newts
            if newtid > tid:
                tid = newtid
            if not isinstance(v, Hash):
                hashtype = get_hash_type_from_data(v)
                typename = HASH_TYPE_TO_XML_TYPE[hashtype]
                yield get_key_value(f'{prefix}{k}-{typename}', v)
                yield b','
            else:
                yield from _serialize(v, f"{prefix}{k}.")
    yield from _serialize(hash_, "")
    yield f'_tid={tid}i'.encode()
    if timestamp > 0:
        yield f' {timestamp}'.encode()


def get_keys_from_result(result, measurement, query):
    keys = [key for key in result.keys() if key[0] == measurement]
    if len(keys) == 0:
        raise RuntimeError(f'No column replied for query {query}')
    return keys


class Results():
    """Parses generic JSON results from a InfluxDb query

    The format is not pretty and is defined here:
    https://docs.influxdata.com/influxdb/v1.7/guides/querying_data/
    """

    def __init__(self, result):
        if isinstance(result, (str, bytes)):
            result = loads(result)
        self.data = result['results']
        if 'error' in self.data[0]:
            raise RuntimeError(self.data[0]['error'])

    def get_series(self):
        for d in self.data:
            yield from d.get('series', [])

    def _get_series_key(self, serie):
        return (serie.get('measurement',
                          serie.get('name', 'results')),
                serie.get('tags', None))

    def keys(self):
        for serie in self.get_series():
            yield self._get_series_key(serie)

    def __iter__(self):
        for key in self.keys():
            yield key, self[key]

    def __getitem__(self, key):
        for serie in self.get_series():
            if key == self._get_series_key(serie):
                yield from serie.get('values', [])

    def get_columns(self, key):
        for serie in self.get_series():
            if key == self._get_series_key(serie):
                return serie.get('columns', [])


class InfluxDbClient():
    def __init__(self, host, port, protocol="http", db="",
                 user="", password="", request_timeout=20):
        self.protocol = protocol
        self.host = host
        self.port = int(port)
        self.db = db
        self.user = user
        self.password = password
        self.request_timeout = request_timeout
        self.client = AsyncHTTPClient(force_instance=False)
        self.basic_auth_header = self._get_basic_auth_header()

    def get_url(self, path):
        return f"{self.protocol}://{self.host}:{self.port}/{path}"

    async def connect(self):
        uri = self.get_url("ping")
        reply = await self.client.fetch(
            uri, method="GET", request_timeout=self.request_timeout)
        assert reply.code == 204, f"{reply.code}: {reply.body}"

    def disconnect(self):
        """http disconnection is handled by tornado"""
        return

    def _build_query_uri(self, args, method):
        uri = self.get_url("query")
        auth_args = {}
        if self.user and self.password:
            auth_args["u"] = self.user
            auth_args["p"] = self.password
        if method == "GET":
            # For GET, both the authentication parameters and the parameters
            # (including the query itself) go in the uri.
            args.update(auth_args)
            uri = f"{uri}?{urlencode(args)}"
        elif method == "POST":
            # For POST, the authentication parameters go in the uri, but the
            # query and any other parameter should go in the body.
            uri = f"{uri}?{urlencode(auth_args)}" if auth_args else uri
        return uri

    def _get_basic_auth_header(self):
        auth_header = None
        if self.user and self.password:
            cred = f'{self.user}:{self.password}'.encode()
            b64_cred = base64.b64encode(cred).decode('utf-8')
            auth_header = {'Authorization': f'Basic {b64_cred}'}
        return auth_header

    async def query(self, args, method="GET"):
        """Sends a query from a set of arguments

        args : dict
            will be passed to the `urlencode`
        method : str
            either "GET" or "POST"
        """
        uri = self._build_query_uri(args, method)
        if method == "GET":
            return await self.client.fetch(
                uri, method=method, request_timeout=self.request_timeout)
        elif method == "POST":
            # For POST requests the user credentials must be URL parameters,
            # not body data.
            query = urlencode(args)
            return await self.client.fetch(
                uri, body=query, method=method,
                request_timeout=self.request_timeout)

    async def write(self, data: bytes | str):
        """Posts the data to the `write` entrypoint of influxDb

        The precision is set statically to uSeconds
        """
        uri = self.get_url("write")
        args = {
            "db": self.db,
            "precision": "u"
        }
        if len(self.user) > 0:
            args["u"] = self.user
        if len(self.password) > 0:
            args["p"] = self.password
        query = urlencode(args)
        ret = await self.client.fetch(
            f"{uri}?{query}", body=data, headers=self.basic_auth_header,
            method="POST", request_timeout=self.request_timeout)
        return ret

    async def write_with_retry(self, data,
                               max_retries=20,
                               retry_wait=250,
                               retry_codes={503, 599}):
        """Posts the data to the `write` entrypoint of InfluxDb. If the write
        fails with a status code that is in a set of failures to retry, retries
        the operation after a wait interval.

        data: bytes or str matching a line protocol
        max_retries: max number of attempts for the write operation
        retry_wait: interval, in milliseconds, to wait before retrying
        retry_codes: http status code that should trigger a retry
        return: dictionary with the keys 'code' (last http code received),
        'reason' (description of last http response received) and 'retried'
        (list of retry codes that triggered retries during the write).
        """
        retries = 0
        reply = {}
        retried = []
        while True:
            try:
                resp = await self.write(data)
                reply['code'] = resp.code
                reply['reason'] = resp.reason
                break
            except HTTPError as e:
                reply['code'] = e.code
                reply['reason'] = e.message
                if e.code not in retry_codes or retries >= max_retries:
                    break
                retries += 1
                retried.append(e.code)
                await asyncio.sleep(retry_wait/1000.0)
        reply['retried'] = retried
        return reply

    async def get_dbs(self):
        """Returns a list of database names"""
        reply = await self.query({"q": "SHOW DATABASES"})
        results = Results(loads(reply.body))
        return [value[0] for value in results[('databases', None)]]

    async def create_db(self, db):
        """Create a database

        db: str
            the DB name
        """
        return await self.query(
            {"q": f"CREATE DATABASE \"{db}\""},
            method="POST")

    async def drop_db(self, db):
        """Deletes a database

        db: str
            the DB name
        """
        return await self.query(
            {"q": f"DROP DATABASE \"{db}\""},
            method="POST")

    async def grant_all_to_user(self, db, usr):
        """Grants all privileges on a database to a user.

        db: str
            the DB name
        usr: str
             the user name
        """
        return await self.query(
            {"q": f"GRANT ALL ON \"{db}\" TO \"{usr}\""},
            method="POST")

    async def db_query(self, query, **kwargs):
        """Queries the database

        query: str
            the influxQL query
        Optional keyword arguments will be passed to `db_query`
            e.g. the `epoch` or the username and password can be passed
            in this dictionary
        """
        verb, _ = query.strip().split(' ', 1)
        if verb.upper() in ('SELECT', 'SHOW'):
            method = "GET"
        else:
            method = "POST"
        query = dedent(query)
        kwargs["db"] = self.db
        kwargs["q"] = query
        try:
            return await self.query(kwargs, method=method)
        except HTTPError as e:
            raise RuntimeError(f"{e} for query {query}")

    async def get_results(self, query, measurement, **kwargs):
        """Queries the database

        query: str
            the influxQL query
        measurement: str
            the measurement queried
        Optional keyword arguments will be passed to `db_query`
            e.g. the `epoch` or the username and password can be passed
            in this dictionary

        returns: a tuple
            r: a generator that can iterate through data
            columns: a list of column names
        """
        reply = await self.db_query(query, **kwargs)
        results = Results(loads(reply.body))
        keys = get_keys_from_result(results, measurement, query)
        return results[keys[0]], results.get_columns(keys[0])

    async def drop_measurement(self, measurement):
        """Drops a measurement (table)

        query: str
            the measurement to be dropped
        """
        await self.db_query(f'DROP MEASUREMENT "{measurement}"')

    async def write_measurement(self, measurement, field_dict,
                                tag_dict=None, timestamp=None):
        """Writes a line into the DB

        using the line protocol
        https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference/

        measurement: string
            the measurement
        field_dict: dict
            a dict containing the key-value pairs for fields
        tag_dict: str
            a dict containing the key-value pairs for tags
        timestamp: int
            optional timestamp in microseconds
        """
        line = get_line_fromdicts(measurement, field_dict, tag_dict, timestamp)
        return await self.write(line)

    async def get_measurements(self):
        """Returns a list of measurements names

        the `db` should be set
        """
        reply = await self.db_query("SHOW MEASUREMENTS")
        results = Results(loads(reply.body))
        return list(results[('measurements', None)])

    async def get_series(self):
        """Returns a list of series names

        the `db` should be set
        """
        reply = await self.db_query("SHOW SERIES")
        results = Results(loads(reply.body))
        return list(results[('series', None)])

    async def get_last_value(self, measurement,
                             field_key, timestamp=None):
        """Returns the last value of a specific field

        measurement: str
            the measurement that will be queried
        field_key: str
            the field key that will be queried
            Can be specified as a regex.
        timestamp: int (optional)
            the maximum timestamp allowed
        """
        timestamp_filter = ""
        if timestamp:
            timestamp_filter = f"WHERE {timestamp:.0f}u <= time "
        query = f"""\
            SELECT /{field_key}/ FROM \"{measurement}\"
            ORDER BY time DESC {timestamp_filter}
            LIMIT 1"""
        reply = await self.db_query(query, epoch='u')
        results = Results(loads(reply.body))
        keys = get_keys_from_result(results, measurement, query)
        return results[keys[0]], results.get_columns(keys[0])

    async def field_has(self, measurement,
                        field_key, condition):
        """Returns true if the condition is met

        measurement: str
            the measurement that will be queried
        field_key: str
            the field key that will be queried
            Can be specified as a regex.
        condition: str
            the influxQL condition for the SELECT query
        """
        query = f"""\
            SELECT COUNT(/{field_key}/) FROM \"{measurement}\"
            WHERE {condition}
            ORDER BY time DESC
            LIMIT 1"""
        reply = await self.db_query(query, epoch='u')
        results = Results(loads(reply.body))
        try:
            keys = get_keys_from_result(results, measurement, query)
        except RuntimeError:
            return False
        # the Results object returns one row, the 0th element is the timestamp
        # which is not valid for a SELECT COUNT query
        for count in next(results[keys[0]])[1:]:
            if count > 0:
                return True
        return False

    async def get_field_values(self, measurement,
                               field_keys, begin, end):
        """Returns a list of values

        measurement: str
            the measurement that will be queried
        field_keys: list or str
            the field key or field keys that will be queried
            Can be specified as a regex.
        begin: int
            the begin time in microseconds
        end: int
            the end time in microseconds

        returns: a tuple
            r: a generator that can iterate through data
            columns: a list of column names
        """
        if not isinstance(field_keys, list):
            field_keys = [field_keys]
        field_keys = [f'/{field}/' for field in field_keys]
        field_keys = ",".join(field_keys)
        query = f"""
            SELECT {field_keys} FROM "{measurement}"
            WHERE {begin:.0f}u <= time AND time <= {end:.0f}u"""
        reply = await self.db_query(query, epoch='u')
        results = Results(loads(reply.body))
        keys = get_keys_from_result(results, measurement, query)
        return results[keys[0]], results.get_columns(keys[0])

    async def get_field_count(self, measurement,
                              field_keys, begin, end):
        """Returns a dictionary of counts indexed by field name

        measurement: str
            the measurement that will be queried
        field_keys: list or str
            the field key or field keys that will be queried.
            Can be specified as a regex.
        begin: int
            the begin time in microseconds
        end: int
            the end time in microseconds
        """
        if not isinstance(field_keys, list):
            field_keys = [field_keys]
        selector = ",".join([f'COUNT(/{field_key}/)'
                             for field_key in field_keys])
        query = f"""
            SELECT {selector} FROM "{measurement}"
            WHERE time >= {begin:.0f}u AND time <= {end:.0f}u"""
        reply = await self.db_query(query, epoch='u')
        results = Results(loads(reply.body))
        keys = get_keys_from_result(results, measurement, query)
        # the Results object returns one row, the 0th element is the timestamp
        # which is not valid for a SELECT COUNT query
        count_row = next(results[keys[0]])
        column_names = [re.sub(r'^count_', r'', k)
                        for k in results.get_columns(keys[0])]
        return {k: count for k, count in zip(column_names[1:], count_row[1:])}

    async def get_field_values_samples(self, measurement,
                                       field_keys, begin, end, number):
        """Returns a sample of size `number` of fields

        measurement: str
            the measurement that will be queried
        field_keys: list or str
            the field key or field keys that will be queried.
            Can be a regex.
        begin: int
            the begin time in microseconds
        end: int
            the end time in microseconds
        """
        if not isinstance(field_keys, list):
            field_keys = [field_keys]
        field_keys = "|".join([field for field in field_keys])
        field_keys = f'SAMPLE(/{field_keys}/, {number})'
        query = f"""\
            SELECT {field_keys}
            FROM "{measurement}"
            WHERE {begin:.0f}u <= time AND time <= {end:.0f}u"""
        reply = await self.db_query(query, epoch='u')
        results = Results(loads(reply.body))
        keys = get_keys_from_result(results, measurement, query)
        return results[keys[0]], results.get_columns(keys[0])

    async def get_field_values_group(self, measurement,
                                     field_keys, begin, end, interval):
        """Returns the last value of one or more field grouped by time interval

        measurement: str
            the measurement that will be queried
        field_keys: list or str
            the field key or field keys that will be queried.
            Can be a regex.
        begin: int
            the begin time in microseconds
        end: int
            the end time in microseconds
        interval: int
            interval in milliseconds (microseconds grouping is not supported)
        """
        if not isinstance(field_keys, list):
            field_keys = [field_keys]
        interval = int(interval // 1e3)
        field_keys = "|".join(field_keys)
        field_keys = f'LAST(/{field_keys}/)'
        query = f"""\
            SELECT {field_keys}
            FROM "{measurement}"
            WHERE {begin:.0f}u <= time AND time <= {end:.0f}u
            GROUP BY time({interval}ms)
            """
        return await self.get_results(query, measurement, epoch='u')

    async def get_fields_mean(self, measurement,
                              field_keys, begin, end, interval):
        """Returns the mean of one or more field grouped by time interval

        measurement: str
            the measurement that will be queried
        field_keys: list or str
            the field key or field keys that will be queried.
            Can be a regex.
        begin: int
            the begin time in microseconds
        end: int
            the end time in microseconds
        interval: int
            interval in milliseconds (microseconds grouping is not supported)
        """
        if not isinstance(field_keys, list):
            field_keys = [field_keys]
        interval = int(interval // 1e3)
        field_keys = "|".join(field_keys)
        field_keys = f'MEAN(/{field_keys}/)'
        query = f"""\
            SELECT {field_keys}
            FROM "{measurement}"
            WHERE {begin:.0f}u <= time AND time <= {end:.0f}u
            GROUP BY time({interval}ms)
            """
        return await self.get_results(query, measurement, epoch='u')
