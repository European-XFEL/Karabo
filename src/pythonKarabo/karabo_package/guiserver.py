import sys
sys.karabo_api = 2

from asyncio import (async, coroutine, start_server, Lock, get_event_loop,
                     sleep, gather, IncompleteReadError)

from karabo import Device, Integer, Assignment
from karabo.hash import Hash
from karabo import openmq
from karabo.p2p import Channel


class GuiServer(Device):
    port = Integer(displayedName="Hostport",
                   description="Local port for this server",
                   assignment=Assignment.OPTIONAL, defaultValue=44444)

    def __init__(self, configuration):
        super().__init__(configuration)

        self.channels = set()
        self.deviceChannels = {}  # which channels is this device visbile in
        self.histories = {}
        self.systemTopology = Hash("device", Hash(), "server", Hash())

    def run(self):
        super().run()
        async(self.run_server())
        async(self.log_handler())

    @coroutine
    def run_server(self):
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, False, False)
        yield from start_server(self.connect_client, port=int(self.port))

    @coroutine
    def connect_client(self, reader, writer):
        channel = Channel(reader, writer)
        channel.lock = Lock()
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
                yield from getattr(self, "handle_" + info["type"])(
                    channel, **{k: v for k, v in info.items() if k != "type"})
        except IncompleteReadError:
            pass
        finally:
            for k in list(self.deviceChannels):
                yield from self.handle_stopMonitoringDevice(channel, k)
            self.channels.remove(channel)

    @coroutine
    def handle_error(self, channel, **kwargs):
        print("ERROR", kwargs)

    @coroutine
    def handle_login(self, channel, username, provider, password,
                     sessionToken):
        yield from self.respond(channel, "systemTopology",
                                systemTopology=self.systemTopology)

    @coroutine
    def handle_reconfigure(self, channel, deviceId, configuration):
        self._ss.emit("call", {deviceId: ["slotReconfigure"]}, configuration)

    @coroutine
    def handle_execute(self, channel, deviceId, command):
        self._ss.emit("call", {deviceId: [command]})

    @coroutine
    def handle_initDevice(self, channel, serverId, **kwargs):
        self._ss.emit("call", {serverId: ["slotStartDevice"]}, Hash(kwargs))

    @coroutine
    def handle_getDeviceConfiguration(self, channel, deviceId):
        conf, deviceId = yield from self.call(deviceId, "slotGetConfiguration")
        yield from self.respond(channel, "deviceConfiguration",
                                deviceId=deviceId, configuration=conf)

    @coroutine
    def handle_killServer(self, channel, serverId):
        self._ss.emit("call", {serverId: ["slotKillServer"]})

    @coroutine
    def handle_killDevice(self, channel, deviceId):
        self._ss.emit("call", {deviceId: ["slotKillDevice"]})

    @coroutine
    def handle_startMonitoringDevice(self, channel, deviceId):
        if deviceId not in self.deviceChannels:
            self.deviceChannels[deviceId] = set()
            self.registerDevice(deviceId)
        self.deviceChannels[deviceId].add(channel)
        yield from self.handle_getDeviceConfiguration(channel, deviceId)

    @coroutine
    def handle_stopMonitoringDevice(self, channel, deviceId):
        channels = self.deviceChannels[deviceId]
        channels.discard(channel)
        if not channels:
            del self.deviceChannels[deviceId]
            self._ss.disconnect(deviceId, "signalChanged", self.slotChanged)
            self._ss.disconnect(deviceId, "signalSchemaUpdated",
                                self.slotSchemaUpdated)

    @coroutine
    def handle_getClassSchema(self, channel, serverId, classId):
        schema, _, _ = yield from self.call(serverId, "slotGetClassSchema",
                                            classId)
        yield from self.respond(channel, "classSchema", serverId=serverId,
                                classId=classId, schema=schema)

    @coroutine
    def handle_getDeviceSchema(self, channel, deviceId):
        schema, id = yield from self.call(deviceId, "slotGetSchema", False)
        yield from self.respond(channel, "deviceSchema", deviceId=id,
                                schema=schema)

    @coroutine
    def handle_getPropertyHistory(self, channel, deviceId, property, t0, t1,
                                  maxNumData=0):
        self._ss.emit("call", {"Karabo_DataLoggerManager_0":
                               "slotGetPropertyHistory"},
                      deviceId, property, Hash(
                          "from", t0, "to", t1, "maxNumData", maxNumData))
        self.histories[deviceId, property] = channel

    @coroutine
    def slotPropertyHistory(self, deviceId, property, data):
        channel = self.histories.pop((deviceId, property), None)
        if channel is None:
            return
        yield from self.respond(channel, "propertyHistory", deviceId=deviceId,
                                property=property, data=data)

    @coroutine
    def slotSchemaUpdated(self, schema, deviceId):
        yield from gather(
            *(self.respond(c, "deviceSchema", deviceId=deviceId, schema=schema)
              for c in self.channels))

    @coroutine
    def slotNotification(self, type, shortMessage, detailedMessage, deviceId):
        yield from gather(
            *(self.respond(c, "notification", deviceId=deviceId,
                           messageType=type, shortMsg=shortMessage,
                           detailedMsg=detailedMessage)
              for c in self.channels))

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
                yield from gather(
                    *(self.respond(c, "log", message=message.data)
                      for c in self.channels))

    @coroutine
    def slotInstanceNew(self, instanceId, info):
        entry = self.updateSystemTopology(instanceId, info)
        for c in self.channels:
            yield from self.respond(c, "instanceNew", topologyEntry=entry)

        if "device" not in info:
            return

        deviceId = next(iter(info["device"]))
        if deviceId in self.deviceChannels:
            self.registerDevice(deviceId)

    @coroutine
    def slotInstanceUpdated(self, instanceId, info):
        entry = self.updateSystemTopology(instanceId, info)
        yield from gather(*(self.respond(c, "instanceUpdated",
                                         topologyEntry=entry)
                            for c in self.channels))

    @coroutine
    def slotInstanceGone(self, instanceId, info):
        type = info["type"]
        yield from gather(
            *(self.respond(c, "instanceGone", instanceId=instanceId,
                           instanceType=type) for c in self.channels))
        self.systemTopology[type].pop(instanceId, None)

    @coroutine
    def slotChanged(self, configuration, deviceId):
        yield from gather(
            *(self.respond(c, "deviceConfiguration",
                           deviceId=deviceId, configuration=configuration)
              for c in self.deviceChannels.get(deviceId, [])))

    @coroutine
    def slotPingAnswer(self, deviceId, info):
        self.updateSystemTopology(deviceId, info)

    def updateSystemTopology(self, instanceId, info):
        type = info["type"]
        ret = Hash(type, Hash())
        ret[type][instanceId] = Hash()
        ret[type][instanceId, ...] = dict(info.items())
        self.systemTopology.merge(ret)
        return ret

    def registerDevice(self, deviceId):
        self._ss.connect(deviceId, "signalChanged", self.slotChanged)
        self._ss.connect(deviceId, "signalSchemaUpdated",
                         self.slotSchemaUpdated)

    @coroutine
    def respond(self, channel, type, **kwargs):
        with (yield from channel.lock):
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
    d.run()
    try:
        loop.run_forever()
    finally:
        loop.close()
