from __future__ import unicode_literals
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo import openmq, hashtypes
from karabo.hash import BinaryParser, Hash
from karabo.hashtypes import String, Int32, Type, Slot
from karabo.enums import AccessLevel, Assignment, AccessMode
from karabo.schema import Schema, Configurable
from karabo.timestamp import Timestamp
from karabo.registry import Registry

from asyncio import async, coroutine, Future, get_event_loop, sleep
from itertools import count
import sys
import time


class Signal(object):
    def __init__(self, *args):
        self.args = args


class Device(Registry):
    @classmethod
    def schematize(cls):
        schema = Schema(cls.__name__, hash)
        for k, v in cls.__dict__.items():
            if isinstance(v, Type):
                h = Hash()
                hash[k] = h
                hash[k, 'nodeType'] = 0
                hash[k, 'valueType'] = v.hashname()
        return schema

    @classmethod
    def register(cls, name, dict):
        super(Device, cls).register(name, dict)
        #globalDevice.slotNewClassAvailable('bla', name, cls.schematize())


def replySlot(slot):
    """ using this decorator you can define a reply slot, everything the
    slot returns is not only sent as a normal reply, but also emitted as
    a signal to the defined slot, back to the sender. """
    def f(g):
        g.replySlot = slot
        return g
    return f


class DeviceServer(Device):
    def __init__(self, deviceId):
        Device.__init__(self, deviceId, type="server", serverId=deviceId,
                        version=23, host="exflpcx18981", visibility=0)


class ConnectionType(object):
    NO_TRACK = 0


class BoundSignal(object):
    def __init__(self, device, name, signal):
        self.device = device
        self.name = name
        self.connected = {}
        self.args = signal.args

    def connect(self, target, slot, dunno):
        self.connected.setdefault(target, set()).add(slot)

    def disconnect(self, target, slot):
        s = self.connected.get(target, None)
        if s is not None:
            s.discard(slot)
            if not s:
                del self.connected[target]

    def __call__(self, *args):
        args = [d.cast(v) for d, v in zip(self.args, args)]
        self.device._ss.emit(self.name, self.connected, *args)


class Client(object):
    def __init__(self, device):
        self._device = device
        self._futures = {}

    @classmethod
    def __dir__(cls):
        return dir(cls)

    def onChanged(self, hash):
        for k, v, a in hash.iterall():
            d = getattr(type(self), k, None)
            if d is not None:
                self.__dict__[d] = v
                f = self._futures.pop(k, None)
                if f is not None:
                    f.set_result((k, v, Timestamp.fromHashAttributes(a)))
        f = self._futures.pop(None, None)
        if f is not None:
            f.set_result(hash)

    def setValue(self, attr, value):
        self._device._ss.emit("call", {self.deviceId: ["slotReconfigure"]},
                              Hash(attr.key, value))


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO('SignalSlotable', '1.0')
class SignalSlotable(Configurable):
    signalHeartbeat = Signal(hashtypes.String(), hashtypes.Int32(),
                             hashtypes.Hash())
    signalChanged = Signal(hashtypes.Hash(), hashtypes.String())

    _deviceId_ = String(
        displayedName="_DeviceID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__")

    deviceId = String(
        displayedName="DeviceID",
        description="The device instance ID uniquely identifies a device "
                    "instance in the distributed system",
        accessMode=AccessMode.READONLY)

    heartbeatInterval = Int32(
        displayedName="Heartbeat interval",
        description="The heartbeat interval",
        assignment=Assignment.OPTIONAL, defaultValue=20,
        requiredAccessLevel=AccessLevel.ADMIN)

    def __init__(self, configuration):
        self.notify_changes = False
        for k in dir(type(self)):
            if isinstance(getattr(self, k, None), Signal):
                setattr(self, k, BoundSignal(self, k, getattr(self, k)))
        super().__init__(configuration)
        self.deviceId = self._deviceId_
        self._ss = get_event_loop().getBroker(self.deviceId,
                                              type(self).__name__)
        self.notify_changes = True
        self.info = Hash("heartbeatInterval", self.heartbeatInterval)

        self.__schemaFutures = {}
        self.__devices = {}
        self.__repliers = {}
        self._tasks = set()

    @coroutine
    def slotPing(self, instanceId, replyIfMe, track=None):
        if replyIfMe:
            if instanceId == self.deviceId:
                return self.info
        else:
            self._ss.emit("call", {instanceId: ["slotPingAnswer"]},
                          self.deviceId, self.info)
        if track and instanceId != self.deviceId:
            self.signalHeartbeat.connect(instanceId, "slotHeartbeat", None)

    @coroutine
    def slotHeartbeat(self, networkId, heartbeatInterval, info):
        pass

    @coroutine
    def slotStopTrackingExistenceOfConnection(self, *args):
        print('receivet stopTracking...', args)

    @coroutine
    def slotDisconnectFromSlot(self, *args):
        print("SDFS", args)

    @coroutine
    def heartbeats(self):
        while True:
            self.signalHeartbeat(self.deviceId, self.heartbeatInterval,
                                 self.info)
            yield from sleep(self.heartbeatInterval)

    @coroutine
    def consume(self):
        self._ss.consumer = openmq.Consumer(
            self._ss.session, self._ss.destination,
            "slotInstanceIds LIKE '%|{}|%' "
            "OR slotInstanceIds LIKE '%|*|%'".format(
                self.deviceId, self.deviceId), False)
        self._ss.emit('call', {'*': ['slotInstanceNew']},
                      self.deviceId, self.info)
        try:
            while True:
                try:
                    message = yield from get_event_loop().run_in_executor(
                        None, self._ss.consumer.receiveMessage, 1000)
                except openmq.Error as e:
                    if e.status != 2103:  # timeout
                        raise
                else:
                    self.async(self.handleMessage(message))
        finally:
            self._ss.emit('call', {'*': ['slotInstanceGone']},
                          self.deviceId, self.info)

    def run(self):
        self.async(self.heartbeats())
        self.async(self.consume())
        super().run()

    def async(self, coro):
        task = async(coro, loop=self._ss.loop)
        self._tasks.add(task)
        task.add_done_callback(lambda task: self._tasks.remove(task))

    def call(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex())
        self._ss.call("call", {device: [target]}, reply, args)
        future = Future(loop=self._ss.loop)
        self.__repliers[reply] = future
        return future

    @coroutine
    def handleMessage(self, message):
        parser = BinaryParser()
        try:
            hash = parser.read(message.data)
        except:
            hash = Hash()
        params = []
        for i in count(1):
            try:
                params.append(hash['a{}'.format(i)])
            except KeyError:
                break
        try:
            replyFrom = message.properties['replyFrom'].decode("ascii")
        except openmq.Error:
            pass
        else:
            f = self.__repliers.get(replyFrom)
            if f is not None:
                f.set_result(params)
            return

        slots = (message.properties['slotFunctions'][1:-1]).decode(
            "utf8").split('||')
        slots = {k: v.split(",") for k, v in (s.split(":") for s in slots)}

        try:
            replyTo = message.properties['replyTo']
        except openmq.Error:
            replyTo = None
        try:
            replyInstanceIds = message.properties['replyInstanceIds']
        except openmq.Error:
            replyInstanceIds = None

        sender = message.properties['signalInstanceId']

        try:
            slots = [getattr(self, s) for s in slots.get(self.deviceId, [])
                     ] + [getattr(self, s) for s in slots.get("*", [])
                          if hasattr(self, s)]
            for slot in slots:
                reply = yield from slot(*params)
                if not isinstance(reply, tuple):
                    reply = reply,
                if hasattr(slot, "replySlot"):
                    self._ss.emit(
                        'call', {message.properties['signalInstanceId'].
                                 decode("utf8"): [slot.replySlot]}, *reply)
                if replyTo is not None and reply != (None,):
                    self._ss.reply(replyTo,
                                   message.properties['signalInstanceId'],
                                   *reply)
                if replyInstanceIds is not None:
                    self._ss.replyNoWait(replyInstanceIds,
                                         message.properties['replyFunctions'],
                                         *reply)
        except Exception as e:
            sys.excepthook(*sys.exc_info())
            print("Slot={}".format(slots))

    def stopEventLoop(self):
        get_event_loop().stop()

    @coroutine
    def slotConnectToSignal(self, signal, target, slot, dunno):
        getattr(self, signal).connect(target, slot, dunno)
        return True

    @coroutine
    def slotDisconnectFromSignal(self, signal, target, slot):
        getattr(self, signal).disconnect(target, slot)
        return True

    @staticmethod
    def create(serverid):
        return SignalSlotable(serverid)

    def updateInstanceInfo(self, info):
        self.info.merge(info)
        self._ss.emit('call', {'*': ['slotInstanceUpdated']},
                      self.deviceId, self.info)

    def setValue(self, attr, value):
        self.__dict__[attr] = value
        if self.notify_changes:
            self.signalChanged(Hash(attr.key, value), self.deviceId)

    def setChildValue(self, key, value):
        if self.notify_changes:
            self.signalChanged(Hash(key, value), self.deviceId)

    @coroutine
    def slotSchemaUpdated(self, schema, deviceId):
        future = self.__schemaFutures.pop(deviceId, None)
        if future is not None:
            future.set_result(schema)

    @coroutine
    def slotChanged(self, configuration, deviceId):
        d = self.__devices.get(deviceId)
        if d is not None:
            d.onChanged(configuration)
        loop = get_event_loop()
        loop.changedFuture.set_result(None)
        loop.changedFuture = Future()

    @coroutine
    def getDevice(self, deviceId):
        ret = self.__devices.get(deviceId)
        if ret is not None:
            return ret

        self._ss.emit("call", {deviceId: ["slotGetSchema"]}, False)
        schema = yield from self.__schemaFutures.setdefault(
            deviceId, Future(loop=self._ss.loop))

        dict = {}
        for k, v, a in schema.hash.iterall():
            if a["nodeType"] == 0:
                d = Type.fromname[a["valueType"]]()
                d.key = k
                dict[k] = d
            elif a["nodeType"] == 1 and a.get("displayType") == "Slot":
                s = Slot()
                s.method = lambda self, name=k: self._device._ss.emit(
                    "call", {self.deviceId: [name]})
                dict[k] = s
        cls = type(schema.name, (Client,), dict)

        self._ss.emit("call", {deviceId: "slotGetConfiguration"})
        self._ss.connect(deviceId, "signalChanged", self.slotChanged)
        ret = cls(self)
        self.__devices[deviceId] = ret
        yield from ret._futures.setdefault(None, Future(loop=self._ss.loop))
        return ret
