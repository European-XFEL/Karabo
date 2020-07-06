from asyncio import get_event_loop, Queue, TimeoutError, wait_for
import hashlib
from zlib import adler32
from base64 import b64encode
from random import choice
from time import time

from karabo.influxdb import (
  InfluxDbClient, lines_fromhash, SEC_TO_USEC, USEC_TO_ATTOSEC)
from karabo.middlelayer import (
    AccessMode, background, coslot, Device, encodeBinary, Int16, Int32, sleep,
    slot, State, String, VectorString)

CONF_TIMEOUT = 5
SCHEMA_TIMEOUT = 5

class InfluxDBLogger(Device):
    """A DataLogger with an InfluxDB server backend

    This device listens to the calls of a DataLoggerManager

    For load-balancing, several of those devices can run in parallel, such that
    each device is archived by one InfluxDB device.
    """
    host = String(displayedName="Host",
                  description="The host name of the InfluxDB",
                  accessMode=AccessMode.INITONLY,
                  defaultValue="localhost")
    port = Int16(displayedName="Port",
                 description="The port to reach InfluxDB",
                 accessMode=AccessMode.INITONLY,
                 defaultValue=8086)

    loggedDevices = Int32(
        displayedName="number of logged devices",
        accessMode=AccessMode.READONLY, defaultValue=0)

    uninitializedDevices = VectorString(
        displayedName="Device to be initialized",
        description="The name of the devices still pending schema and "
        "configuration update",
        accessMode=AccessMode.READONLY)

    modulo = Int32(
        displayedName="Parallel Instances",
        description="The number of InfluxDB instances running in parallel "
                    "for load balancing",
        accessMode=AccessMode.INITONLY, defaultValue=1)

    number = Int32(
        displayedName="Parallel Instance Number",
        description="The unique number of this device of the parallel "
                    "InfluxDB instances",
        accessMode=AccessMode.INITONLY, defaultValue=0)

    messages = Int32(
        displayedName="Messages",
        description="Total number of messages archived",
        accessMode=AccessMode.READONLY, defaultValue=0)

    client = None

    def __init__(self, config):
        super().__init__(config)
        self.tracked = dict()
        self.queue = Queue()
        self.schema_queue = Queue()
        self.schema_task = None
        self.client = InfluxDbClient(self.host, self.port)
        topic = get_event_loop().topic
        self.client.db = topic

    async def onInitialization(self):
        await self.client.connect()
        if self.client.db not in (await self.client.get_dbs()):
            await self.client.create_db(self.client.db)
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, 0, False)
        await self.client.connect()
        self.schema_task = background(self.poll_schemas())
        background(self.push_to_db_action())
        background(self.push_schemas_action())

    @coslot
    async def slotInstanceNew(self, instanceId, info):
        self.updateInfo(instanceId, info)
        await super().slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceGone(self, deviceId, info):
        super().slotInstanceGone(deviceId, info)
        dev = self.tracked.pop(deviceId, None)
        if dev is not None:
            self._ss.disconnect(deviceId, "signalChanged", self.onChanged)
            self._ss.disconnect(deviceId, "signalStateChanged",
                                self.onChanged)
            self._ss.disconnect(deviceId, "signalSchemaUpdated",
                                self.onSchemaUpdate)
            line = (deviceId.encode('utf-8'),
                    b'__EVENTS',
                    b',"type"="LOG-" "karabo_user"="."')
            self.queue.put_nowait(b''.join(line))

    @slot
    def slotPingAnswer(self, deviceId, info):
        self.updateInfo(deviceId, info)

    def updateInfo(self, deviceId, info):
        # this datalogger alternative does not rely on a data logger manager
        # but it calculates the adler32 hash of any deviceId being instantiated
        # and log if the modulo of the hash number matches the self.number
        # it is configured for. If a system has 8 loggers, `modulo` should be
        # set to 8 and number should be set from 0 to 7 in each dataLogger 
        modulo = adler32(deviceId.encode("utf8")) % self.modulo.value
        if (modulo != self.number):
                return
        if info.get("archive") and deviceId != self.deviceId:
            self.tracked[deviceId] = False
            self._ss.connect(deviceId, "signalChanged", self.onChanged)
            self._ss.connect(deviceId, "signalStateChanged", self.onChanged)
            self._ss.connect(deviceId, "signalSchemaUpdated",
                             self.onSchemaUpdate)
            line = (deviceId.encode('utf-8'),
                    b'__EVENTS',
                    b',type="LOG+" karabo_user="."')
            self.queue.put_nowait(b''.join(line))
            self.update_tracked()

    def update_tracked(self):
        self.loggedDevices = len(self.tracked.keys())
        self.uninitializedDevices = [
            dev for dev, received in self.tracked.items() if not received]
        
    @slot
    def onChanged(self, data, deviceId):
        if deviceId == self.deviceId:
            return
        self.queue.put_nowait(b"".join(lines_fromhash(deviceId, data)))

    @slot
    def onSchemaUpdate(self, schema, deviceId):
        # the schema is deserialized and now reserialized.
        b = encodeBinary(schema)
        b_schema = b64encode(b)
        timestamp = int(time() * SEC_TO_USEC)
        self.schema_queue.put_nowait((deviceId, b_schema, timestamp))

    async def push_schemas_action(self):
        while True:
            await self.push_one_schema()

    async def push_one_schema(self):
        deviceId, b_schema, timestamp = (await self.schema_queue.get())
        digest = hashlib.sha1(b_schema).hexdigest()
        has_schema = await self.client.field_has(
            f'{deviceId}__SCHEMAS', "schema", f'"digest" = \'"{digest}"\'')
        if not has_schema:
            # save the schema
            line = (deviceId.encode('utf-8'),
                    b'__SCHEMAS',
                    b',digest="',
                    digest.encode('utf-8').replace(
                        b'\\', b'\\\\').replace(b'"', br'\"'),
                    b'" schema="', b_schema, b'" ',
                    str(timestamp).encode('utf-8'))
            self.queue.put_nowait(b''.join(line))
        # save the schema incoming event irregardless
        line = (deviceId.encode('utf-8'),
                b'__EVENTS',
                b',type="SCHEMA" schema_digest="',
                digest.encode('utf-8').replace(
                    b'\\', b'\\\\').replace(b'"', br'\"'),
                b'" ', str(timestamp).encode('utf-8'))
        self.queue.put_nowait(b''.join(line))
        conf, _ = await self.call(deviceId, 'slotGetConfiguration')
        self.onChanged(conf, deviceId)

    async def push_to_db_action(self):
        self.state = State.MONITORING
        while True:
            await self.client.connect()
            try:
                while True:
                    data = [(await self.queue.get())]
                    while not self.queue.empty():
                        data.append(self.queue.get_nowait())
                    data = b"\n".join(data)
                    await self.client.write(data)
                    self.messages = self.messages + 1
            finally:
                self.client.disconnect()
            await sleep(1)  # treat InfluxDB nicely

    def ensure_poll(self):
        if self.schema_task is None:
            self.schema_task = background(self.poll_schemas())

    async def poll_one(self):
        if len(self.uninitializedDevices.value) == 0:
            return False
        try:
            dev = choice(self.uninitializedDevices)
            schema, _ = await wait_for(
                self.call(dev, "slotGetSchema", False), timeout=SCHEMA_TIMEOUT
            )
            self.onSchemaUpdate(schema, dev)
            conf, _ = await wait_for(
                self.call(dev, "slotGetConfiguration"), timeout=CONF_TIMEOUT
            )
            self.onChanged(conf, dev)
            self.tracked[dev] = True
            self.update_tracked()
        finally:
            # continue polling
            return True

    async def poll_schemas(self):
        poll = True
        while poll:
            poll = await self.poll_one()
        self.schema_task = None
