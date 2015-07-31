import karabo

from asyncio import (async, coroutine, Future, start_server, get_event_loop,
                     IncompleteReadError, open_connection, sleep, wait_for)
from functools import wraps
import os
from weakref import WeakValueDictionary

from karabo.device_client import DeviceClientBase
from karabo.api import Int, Assignment
from karabo import openmq, KaraboError
from karabo.enums import Unit, MetricPrefix
from karabo.hash import Hash
from karabo.p2p import Channel
from karabo.signalslot import slot


def parallel(f):
    f = coroutine(f)

    @wraps(f)
    def wrapper(self, *args, **kwargs):
        return async(f(self, *args, **kwargs))
    return wrapper


class Subscription:
    def __init__(self, channelName, parent):
        self.instance, self.name = channelName.split(":")
        self.parent = parent
        self.triggered = {}
        self.subscriptions = set()

    @coroutine
    def start(self):
        ok, info = yield from wait_for(self.parent.call(
            self.instance, "slotGetOutputChannelInformation", self.name,
            os.getpid()), 1)
        if not ok:
            raise KaraboError('no information for channel "{}"'.
                              format(self.name))
        self.channel = Channel(
            *(yield from open_connection(info["hostname"], int(info["port"]))))

    @coroutine
    def trigger(self):
        h = Hash("reason", "hello", "instanceId", self.parent.deviceId,
                 "memoryLocation", "remote",
                 "dataDistribution", "copy",
                 "onSlowness", "drop")
        r = ["endOfStream"]
        while "endOfStream" in r:
            self.channel.writeHash(h, "XML")
            r = yield from self.channel.readHash("XML")
            bt = yield from self.channel.readBytes()
        yield from sleep(self.parent.delayOnInput / 1000)
        l = r["byteSizes"][0]
        data = Hash.decode(bt[:l], "Bin")
        for f in self.triggered.values():
            f.set_result(data)
        self.triggered = {}
        return True

    def subscribe(self, channel):
        self.subscriptions.add(channel)

    def unsubscribe(self, channel):
        self.subscriptions.discard(channel)
        f = self.triggered.pop(channel, None)
        if f is not None:
            f.set_result(None)

    @coroutine
    def update(self, channel):
        if channel not in self.subscriptions:
            return None
        trigger = not self.triggered
        future = self.triggered[channel] = Future()
        if trigger:
            yield from self.trigger()
        return (yield from future)


class GuiServer(DeviceClientBase):
    port = Int(displayedName="Hostport",
               description="Local port for this server",
               assignment=Assignment.OPTIONAL, defaultValue=44444)

    delayOnInput = Int(
        displayedName="Delay on Input channel",
        description="Some delay before informing output channel about "
                    "readiness for next data.",
        assignment=Assignment.OPTIONAL, defaultValue=500,
        unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MILLI)

    def __init__(self, configuration):
        super().__init__(configuration)

        self.channels = set()
        self.deviceChannels = {}  # which channels is this device visbile in
        self.subscriptions = WeakValueDictionary()

    def run(self):
        async(self.log_handler())
        super().run()
        async(self.run_server())

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
            for v in self.subscriptions.values():
                v.unsubscribe(channel)

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
    def handle_subscribeNetwork(self, channel, channelName, subscribe):
        subs = self.subscriptions.get(channelName)
        if not subscribe:
            if subs is not None:
                subs.unsubscribe(channel)
            return

        if subs is None:
            subs = self.subscriptions[channelName] = \
                Subscription(channelName, self)
            yield from subs.start()
        subs.subscribe(channel)
        while True:
            data = yield from subs.update(channel)
            if data is None:
                return
            self.respond(channel, "networkData", name=channelName, data=data)
            yield from channel.drain()

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

    @parallel
    def handle_getPropertyHistory(self, channel, deviceId, property, t0, t1,
                                  maxNumData=0):
        id = "DataLogger-{}".format(deviceId)
        if id not in self.loggerMap:
            self.loggerMap = yield from self.call(
                "Karabo_DataLoggerManager_0", "slotGetLoggerMap")
            if id not in self.loggerMap:
                raise KaraboError('no logger for device "{}"'.
                                  format(deviceId))
        reader = "DataLogReader-{}".format(self.loggerMap[id])
        deviceId, property, data = yield from self.call(
            reader, "slotGetPropertyHistory", deviceId, property,
            Hash("from", t0, "to", t1, "maxNumData", maxNumData))
        self.respond(channel, "propertyHistory", deviceId=deviceId,
                     property=property, data=data)

    @parallel
    def handle_getAvailableProjects(self, channel):
        projects = yield from self.call("Karabo_ProjectManager",
                                        "slotGetAvailableProjects")
        self.respond(channel, "availableProjects", availableProjects=projects)

    @parallel
    def handle_newProject(self, channel, author, name, data):
        name, success, data = yield from self.call(
            "Karabo_ProjectManager", "slotNewProject", author, name, data)
        self.respond(channel, "projectNew", name=name, success=success,
                     data=data)

    @parallel
    def handle_loadProject(self, channel, user, name):
        name, metaData, data = yield from self.call(
            "Karabo_ProjectManager", "slotLoadProject", user, name)
        self.respond(channel, "projectLoaded", name=name, metaData=metaData,
                     buffer=data)

    @parallel
    def handle_saveProject(self, channel, user, name, data):
        name, success, data = yield from self.call(
            "Karabo_ProjectManager", "slotSaveProject", user, name)
        self.respond(channel, "projectSaved", name=name, success=success,
                     data=data)

    @parallel
    def handle_closeProject(self, channel, user, name):
        name, success, data = yield from self.call(
            "Karabo_ProjectManager", "slotCloseProject", user, name)
        self.respond(channel, "projectLoaded", name=name, success=success,
                     data=data)

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
    d.startInstance()
    try:
        loop.run_forever()
    finally:
        loop.close()
