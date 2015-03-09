import karabo
karabo.api_version = 2

from asyncio import (async, coroutine, start_server, get_event_loop,
                     IncompleteReadError)
from functools import wraps

from karabo.device_client import DeviceClientBase
from karabo import Integer, Assignment, openmq
from karabo.hash import Hash
from karabo.p2p import Channel
from karabo.signalslot import slot


def parallel(f):
    f = coroutine(f)
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        return self.async(f(self, *args, **kwargs))
    return wrapper


class GuiServer(DeviceClientBase):
    port = Integer(displayedName="Hostport",
                   description="Local port for this server",
                   assignment=Assignment.OPTIONAL, defaultValue=44444)

    def __init__(self, configuration):
        super().__init__(configuration)

        self.channels = set()
        self.deviceChannels = {}  # which channels is this device visbile in
        self.histories = {}

    def run(self):
        self.async(self.log_handler())
        super().run()
        self.async(self.run_server())

    @coroutine
    def run_server(self):
        yield from start_server(self.connect_client, port=int(self.port))

    @coroutine
    def connect_client(self, reader, writer):
        channel = Channel(reader, writer)
        props = get_event_loop().connection.properties
        brokerInfo = Hash("type", "brokerInformation",
                          "host", props["MQBrokerHostName"],
                          "port", props["MQBrokerHostPort"],
                          "topic", self._ss.destination.name)
        channel.writeHash(brokerInfo, "Bin")

        self.channels.add(channel)
        try:
            while True:
                info = yield from channel.readHash("Bin")
                getattr(self, "handle_" + info["type"])(
                    channel, **{k: v for k, v in info.items() if k != "type"})
        except IncompleteReadError:
            pass
        finally:
            for k in list(self.deviceChannels):
                self.handle_stopMonitoringDevice(channel, k)
            self.channels.remove(channel)

    def handle_error(self, channel, **kwargs):
        print("ERROR", kwargs)

    def handle_login(self, channel, username, provider, password,
                     sessionToken, pid, version, host):
        self.respond(channel, "systemTopology",
                     systemTopology=self.systemTopology)

    def handle_reconfigure(self, channel, deviceId, configuration):
        self._ss.emit("call", {deviceId: ["slotReconfigure"]}, configuration)

    def handle_execute(self, channel, deviceId, command):
        self._ss.emit("call", {deviceId: [command]})

    def handle_initDevice(self, channel, serverId, **kwargs):
        self._ss.emit("call", {serverId: ["slotStartDevice"]}, Hash(kwargs))

    @parallel
    def handle_getDeviceConfiguration(self, channel, deviceId):
        conf, deviceId = yield from self.call(deviceId, "slotGetConfiguration")
        self.respond(channel, "deviceConfiguration",
                     deviceId=deviceId, configuration=conf)

    def handle_killServer(self, channel, serverId):
        self._ss.emit("call", {serverId: ["slotKillServer"]})

    def handle_killDevice(self, channel, deviceId):
        self._ss.emit("call", {deviceId: ["slotKillDevice"]})

    def handle_startMonitoringDevice(self, channel, deviceId):
        if deviceId not in self.deviceChannels:
            self.deviceChannels[deviceId] = set()
            self.registerDevice(deviceId)
        self.deviceChannels[deviceId].add(channel)
        self.handle_getDeviceConfiguration(channel, deviceId)

    def handle_stopMonitoringDevice(self, channel, deviceId):
        channels = self.deviceChannels[deviceId]
        channels.discard(channel)
        if not channels:
            del self.deviceChannels[deviceId]
            self._ss.disconnect(deviceId, "signalChanged", self.slotChanged)
            self._ss.disconnect(deviceId, "signalSchemaUpdated",
                                self.slotSchemaUpdated)

    @parallel
    def handle_getClassSchema(self, channel, serverId, classId):
        schema, _, _ = yield from self.call(serverId, "slotGetClassSchema",
                                            classId)
        self.respond(channel, "classSchema", serverId=serverId,
                     classId=classId, schema=schema)

    @parallel
    def handle_getDeviceSchema(self, channel, deviceId):
        schema, id = yield from self.call(deviceId, "slotGetSchema", False)
        self.respond(channel, "deviceSchema", deviceId=id, schema=schema)

    def handle_getPropertyHistory(self, channel, deviceId, property, t0, t1,
                                  maxNumData=0):
        self._ss.emit("call", {"Karabo_DataLoggerManager_0":
                               "slotGetPropertyHistory"},
                      deviceId, property, Hash(
                          "from", t0, "to", t1, "maxNumData", maxNumData))
        self.histories[deviceId, property] = channel

    @slot
    def slotPropertyHistory(self, deviceId, property, data):
        channel = self.histories.pop((deviceId, property), None)
        if channel is None:
            return
        self.respond(channel, "propertyHistory", deviceId=deviceId,
                     property=property, data=data)

    @slot
    def slotSchemaUpdated(self, schema, deviceId):
        for c in self.channels:
            self.respond(c, "deviceSchema", deviceId=deviceId, schema=schema)

    @slot
    def slotNotification(self, type, shortMessage, detailedMessage, deviceId):
        for c in self.channels:
            self.respond(c, "notification", deviceId=deviceId,
                         messageType=type, shortMsg=shortMessage,
                         detailedMsg=detailedMessage)

    @coroutine
    def log_handler(self):
        consumer = openmq.Consumer(self._ss.session, self._ss.destination,
                                   "target = 'log'", False)
        while True:
            try:
                message = yield from get_event_loop().run_in_executor(
                    None, consumer.receiveMessage, 1000)
            except openmq.Error as e:
                if e.status != 2103:
                    raise
            else:
                for c in self.channels:
                    self.respond(c, "log", message=message.data)

    def updateSystemTopology(self, instanceId, info, task):
        entry = super().updateSystemTopology(instanceId, info, task)
        if task is not None:
            for c in self.channels:
                self.respond(c, task, topologyEntry=entry)
        return entry

    @slot
    def slotInstanceNew(self, instanceId, info):
        super().slotInstanceNew(instanceId, info)

        if "device" not in info:
            return

        deviceId = next(iter(info["device"]))
        if deviceId in self.deviceChannels:
            self.registerDevice(deviceId)

    @slot
    def slotInstanceGone(self, instanceId, info):
        for c in self.channels:
            self.respond(c, "instanceGone", instanceId=instanceId,
                         instanceType=info["type"])
        return super().slotInstanceGone(instanceId, info)

    @slot
    def slotChanged(self, configuration, deviceId):
        for c in self.deviceChannels.get(deviceId, []):
            self.respond(c, "deviceConfiguration", deviceId=deviceId,
                         configuration=configuration)

    def registerDevice(self, deviceId):
        self._ss.connect(deviceId, "signalChanged", self.slotChanged)
        self._ss.connect(deviceId, "signalSchemaUpdated",
                         self.slotSchemaUpdated)

    def respond(self, channel, type, **kwargs):
        h = Hash("type", type)
        for k, v in kwargs.items():
            h[k] = v
        channel.writeHash(h, "Bin")

if __name__ == "__main__":
    from asyncio import set_event_loop
    from karabo.eventloop import EventLoop

    loop = EventLoop()
    set_event_loop(loop)

    d = GuiServer({"_deviceId_": "GuiServer"})
    async(d.run_async())
    try:
        loop.run_forever()
    finally:
        loop.close()
