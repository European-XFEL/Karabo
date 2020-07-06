from asyncio import Future, get_event_loop
from datetime import datetime, timezone
from time import strptime
from random import randrange
from base64 import b64decode

from numpy import uint64

from karabo.middlelayer import (
    AccessMode, background, coslot, decodeBinary, Device, Hash,
    HashType, Int16, Schema, SchemaHashType, Signal, String, Type)
from karabo.native.data.serializers import BinaryParser
from karabo.influxdb import (
  InfluxDbClient, SEC_TO_USEC, USEC_TO_ATTOSEC)


class SchemaBinaryParser(BinaryParser):
    def read(self, data):
        self.pos = 0
        self.data = data
        return SchemaHashType.read(self)


def decodeSchemaBinary(data):
    parser = SchemaBinaryParser()
    return parser.read(data)


def parsetime(strtime):
    parts = strtime.split(".")
    if len(parts) > 1:
        s_date, s_frac = parts
    else:
        s_date = parts[0]
        s_frac = '0'
    # parse date
    fmt = "%Y-%m-%dT%H:%M:%S"
    if s_date.endswith('Z'):
        s_date = s_date[:-1]
    dt = datetime(*(strptime(s_date, fmt)[0:6]), tzinfo=timezone.utc)
    secs = dt.timestamp()
    # parse fraction
    secs += float(f"0.{s_frac}")
    return 1e6 * secs  # time has us precision


class DataLogReader(Device):
    """Read back the data from the InfluxDB

    This is the data log reader that reads back data from the InfluxDB
    """
    host = String(displayedName="Host",
                  description="The host name of the InfluxDB",
                  accessMode=AccessMode.INITONLY,
                  defaultValue="localhost")
    port = Int16(displayedName="Port",
                 description="The port to reach InfluxDB",
                 accessMode=AccessMode.INITONLY,
                 defaultValue=8086)
    user = String(displayedName="InfluxUser",
                  description="The InfluxDB user name",
                  accessMode=AccessMode.INITONLY,
                  defaultValue="")
    password = String(displayedName="InfluxPassword",
                      description="The password for the InfluxDB user",
                      accessMode=AccessMode.INITONLY,
                      defaultValue="")

    client = None

    async def onInitialization(self):
        self.client = InfluxDbClient(self.host, self.port.value,
                                     user=self.user, password=self.password)
        await self.client.connect()
        topic = get_event_loop().topic
        if topic not in (await self.client.get_dbs()):
            await self.client.create_db(topic)
        self.client.db = topic

    @coslot
    async def slotGetPropertyHistory(self, deviceId, key, times):
        begin = parsetime(times["from"])
        end = parsetime(times["to"])
        length = times["maxNumData"]
        ret = []
        try:
            count_dict = await self.client.get_field_count(
                deviceId, f'^{key}-.*$', begin, end)
        except RuntimeError:
            # no data will raise a Runtime Error
            return deviceId, key, ret
        n = sum((v for v in count_dict.values() if v is not None))
        set_tid = True
        # check if the points in this range are less than the max
        db_keys = set(count_dict.keys())
        if n <= length:
            rows, cols = await self.client.get_field_values(
                deviceId, f'^{key}-.*$|_tid', begin, end)
            cols = [col.replace('"', '') for col in cols]
        elif ("VECTOR" in db_keys or
              "STRING" in db_keys or
              "BOOL" in db_keys or
              "HASH" in db_keys):
            # the DB cannot average this types (string and bool)
            # we ask here for samples
            request_length = length * len(db_keys)
            results_gen, cols = await self.client.get_field_values_samples(
                deviceId, f'^{key}-.*$|_tid', begin, end, request_length)
            # we ask the DB for samples of _tid and the values
            # the column names will be something like
            # 'value-Type', 'value-Type2', '_tid'
            # The rows are sparse!
            # we need to massage the data to fit into ourself

            # we select the columns that has 'value' in the name
            key_cols = [i for i, col in enumerate(cols) if key in col]
            # select rows that have at lease a not None 'value' 
            rows = [row for row in results_gen
                    if any([row[i] is not None for i in key_cols])]
            # The algorithm can return more data that requested
            # (N samples per type), therefore we reduce the data
            # if necessary
            if len(rows) > length:
                selection = [randrange(0, len(rows))
                             for _ in range(len(rows)-request_length)]
                selection.sort(reverse=True)
                for i in selection:
                    rows.pop(i)
            cols = [col.replace('sample_', '').replace('"', '')
                    for col in cols]
        else:
            # we will receive an averaged field, the trainId is not valid
            set_tid = False
            interval = (end - begin) / (length)
            rows, cols = await self.client.get_fields_mean(
                deviceId, f'^{key}-.*$', begin, end, interval)
            cols = [col.replace('mean_', '').replace('"', '')
                    for col in cols]

        if set_tid:
            tid_col = [i for i, col in enumerate(cols) if col == '_tid'][0]
            cols.pop(tid_col)  # train_id
        cols.pop(0)  # time
        for vals in rows:
            if set_tid:
                tid = vals.pop(tid_col)
            else:
                tid = None
            time = float(vals.pop(0))
            d = Hash()
            for col, v in zip(cols, vals):
                if v is not None:
                    karabo_type = col.split('-')[-1]
                    descr = Type.fromname[karabo_type]
                    d['v'] = descr.fromstring(str(v))
                    break
            else:
                # the sampling has grouped into an interval with no data
                continue
            d["v", "tid"] = uint64(tid or 0)
            d["v", "sec"] = uint64(time // SEC_TO_USEC)
            d["v", "frac"] = uint64((time % SEC_TO_USEC) * USEC_TO_ATTOSEC)
            ret.append(d)
        return deviceId, key, ret

    @coslot
    async def slotGetConfigurationFromPast(self, deviceId, timepoint):
        latest = parsetime(timepoint)
        # get the latest digest
        measurement = f"{deviceId}__EVENTS"
        query = f"""\
            SELECT LAST(karabo_user) FROM "{measurement}"
            WHERE "type" = '"LOG+"' AND
            time <= {latest:.0f}u"""
        try:
            r, _ = await self.client.get_results(query, measurement, epoch='u')
            lastlogin, _ = next(r)
        except RuntimeError:
            lastlogin = 0
        query = f"""\
            SELECT LAST(karabo_user) FROM "{measurement}"
            WHERE "type" = '"LOG-"' AND
            time <= {latest:.0f}u"""
        try:
            r, _ = await self.client.get_results(query, measurement, epoch='u')
            lastlogout, _ = next(r)
        except RuntimeError:
            lastlogout = 0
        configAtTimestamp = lastlogout < lastlogin
        query = f"""\
            SELECT LAST(schema_digest) FROM "{measurement}__EVENTS"
            WHERE "type" = '"SCHEMA"' AND
            time <= {latest:.0f}u"""
        r, _ = await self.client.get_results(query, measurement, epoch='u')
        _, digest = next(r)
        measurement = f"{deviceId}__SCHEMAS"
        query = f"""\
            SELECT LAST(schema) FROM "{measurement}"
            WHERE "digest" = '"{digest}"' """
        r, _ = await self.client.get_results(query, measurement, epoch='u')
        _, data = next(r)
        buf = b64decode(data)
        schemah = decodeSchemaBinary(buf)
        conf = Hash()
        measurement = f"{deviceId}"
        latest_ts = 0

        async def recurse_schema(s, prefix):
            nonlocal conf, latest_ts
            for k, v, a in s.iterall():
                if isinstance(v, Hash):
                    await recurse_schema(v, f"{prefix}{k}.")
                else:
                    prop = f'{prefix}{k}-{a["valueType"]}'
                    query = f"""\
                        SELECT LAST(/{prop}/) FROM "{measurement}"
                        WHERE time <= {latest:.0f}u"""
                    try:
                        r, _ = await self.client.get_results(
                            query, measurement, epoch='u')
                    except RuntimeError:
                        continue
                    ts, v = next(r)
                    if ts > latest_ts:
                        latest_ts = ts
                    key = f'{prefix}{k}'
                    if isinstance(v, str):
                        try:
                            v = Type.fromname[a["valueType"]].fromstring(v)
                        except AttributeError as e:
                            # temporarily ignore keys that cannot be parsed
                            continue
                    conf[key] = v
                    # us in a s (1e6)
                    conf[key, "sec"] = uint64(ts // SEC_TO_USEC)
                    # modulo us in a s (1e6) * atto-s in a us (1e12)
                    conf[key, "frac"] = uint64((ts % SEC_TO_USEC) * USEC_TO_ATTOSEC)
                    conf[key, "tid"] = uint64(0)                    
        await recurse_schema(schemah, "")
        schema = Schema(conf['classId'], hash=schemah)
        configTimeStr = datetime.fromtimestamp(latest_ts / SEC_TO_USEC).isoformat()
        return conf, schema, configAtTimestamp, configTimeStr


class DataLoggerManager(Device):
    """Data Logger Manager Adapter for the DeviceClient

    This data logger manager fulfills the role of informing a DeviceClient
    about the data log reader it should read from.
    It currently returns a map pointing to the same Id.
    """
    host = String(displayedName="Host",
                  description="The host name of the InfluxDB",
                  accessMode=AccessMode.INITONLY,
                  defaultValue="localhost")
    port = Int16(displayedName="Port",
                 description="The port to reach InfluxDB",
                 accessMode=AccessMode.INITONLY,
                 defaultValue=8086)

    logReaderServerId = String(displayedName="ServerId of Log Reader",
                               description="ServerId of the log Reader",
                               accessMode=AccessMode.INITONLY,
                               defaultValue="")

    signalLoggerMap = Signal(HashType())

    client = None
    have_deviceIds = None
    loggerMap = None

    def __init__(self, config):
        super().__init__(config)
        self.loggerMap = Hash()
        self.have_deviceIds = Future()

    async def onInitialization(self):
        if self.logReaderServerId == "":
            self.logReaderServerId = self.serverId
        self.client = InfluxDbClient(self.host, self.port.value)
        await self.client.connect()
        topic = get_event_loop().topic
        if topic not in (await self.client.get_dbs()):
            await self.client.create_db(topic)
        self.client.db = topic
        background(self.read_deviceIds())

    @coslot
    async def slotGetLoggerMap(self):
        if not self.have_deviceIds.done():
            await self.have_deviceIds
        return self.loggerMap

    async def read_deviceIds(self):
        """Read the available device IDs from the database.

        This is run automatically on startup, but may be re-read if necessary.
        """
        measurements = await self.client.get_measurements()
        tables = (table[0]
                  for table in measurements
                  if (not table[0].endswith("__SCHEMAS") and
                      not table[0].endswith("__EVENTS")))
        self.loggerMap.clear()
        for dev in tables:
            self.loggerMap["DataLogger-" + dev] = self.logReaderServerId.value
        if not self.have_deviceIds.done():
            self.have_deviceIds.set_result(None)
        self.signalLoggerMap(self.loggerMap)
