from __future__ import absolute_import, unicode_literals
from karabo.hashtypes import Type, Slot
from karabo.hash import Hash, BinaryWriter
from karabo import openmq

from asyncio import (coroutine, Future, get_event_loop, set_event_loop,
                     SelectorEventLoop, Task)
from copy import copy
from concurrent.futures import ThreadPoolExecutor
import getpass
from itertools import count
import sys
import weakref


class Broker:
    def __init__(self, loop, deviceId, classId):
        self.loop = loop
        self.connection = loop.connection
        self.session = openmq.Session(self.connection, False, 1, 0)
        self.destination = openmq.Destination(self.session,
                                              getpass.getuser(), 1)
        self.producer = openmq.Producer(self.session, self.destination)
        self.deviceId = deviceId
        self.classId = classId
        self.repliers = {}

    def send(self, p, args):
        hash = Hash()
        for i, a in enumerate(args):
            hash['a{}'.format(i + 1)] = a
        m = openmq.BytesMessage()
        m.data = hash.encode("Bin")
        p['signalInstanceId'] = self.deviceId
        p['__format'] = 'Bin'
        m.properties = p
        self.producer.send(m, 1, 4, 1000)

    def call(self, signal, targets, reply, args):
        p = openmq.Properties()
        p['signalFunction'] = signal
        if targets:
            p['slotInstanceIds'] = ('|' + '||'.join(t for t in targets) + '|'
                                    ).encode("utf8")
            p['slotFunctions'] = ('|' + '||'.join(
                "{}:{}".format(k, ",".join(v)) for k, v in targets.items()) +
                '|').encode("utf8")
        else:
            p['slotInstanceIds'] = b'__none__'
            p['slotFunctions'] = b'__none__'
        if reply is not None:
            p['replyTo'] = reply
        p['hostname'] = "exflpcx18981"
        p['classId'] = self.classId
        self.send(p, args)

    def log(self, message):
        p = openmq.Properties()
        p["target"] = "log"
        m = openmq.TextMessage()
        m.data = message
        m.properties = p
        self.producer.send(m, 1, 4, 1000)

    def emit(self, signal, targets, *args):
        self.call(signal, targets, None, args)

    def reply(self, message, reply):
        sender = message.properties['signalInstanceId']

        if not isinstance(reply, tuple):
            reply = reply,

        replyTo = message.properties.get('replyTo')
        if replyTo is not None:
            p = openmq.Properties()
            p['replyFrom'] = replyTo
            p['signalFunction'] = "__reply__"
            p['slotInstanceIds'] = b'|' + sender + b'|'
            self.send(p, reply)

        replyInstanceIds = message.properties.get('replyInstanceIds')
        if replyInstanceIds is not None:
            p = openmq.Properties()
            p['signalFunction'] = "__replyNoWait__"
            p['slotInstanceIds'] = replyInstanceIds
            p['slotFunctions'] = message.properties['replyFunctions']
            self.send(p, reply)

    def connect(self, deviceId, signal, slot):
        self.emit("call", {deviceId: ["slotConnectToSignal"]}, signal,
                  slot.__self__.deviceId, slot.__name__, 0)

    def disconnect(self, deviceId, signal, slot):
        self.emit("call", {deviceId: ["slotDisconnectFromSignal"]}, signal,
                  slot.__self__.deviceId, slot.__name__)

    def registerSlot(self, x):
        """ legacy code. slots don't need to be registered """
        return

    @coroutine
    def consume(self, device):
        consumer = openmq.Consumer(
            self.session, self.destination,
            "slotInstanceIds LIKE '%|{0.deviceId}|%' "
            "OR slotInstanceIds LIKE '%|*|%'".format(self), False)
        info = device.info
        try:
            while True:
                device = weakref.ref(device)
                try:
                    message = yield from get_event_loop().run_in_executor(
                        None, consumer.receiveMessage, 1000)
                except openmq.Error as e:
                    if e.status == 2103:  # timeout
                        continue
                    else:
                        raise
                finally:
                    device = device()
                    if device is None:
                        return
                info = device.info
                try:
                    slots, params = self.decodeMessage(message)
                except:
                    device.logger.exception("malformated message")
                    continue
                try:
                    slots = [getattr(device, s)
                             for s in slots.get(self.deviceId, [])] + \
                            [getattr(device, s) for s in slots.get("*", [])
                             if hasattr(device, s)]
                except AttributeError:
                    device.logger.exception("slot does not exist")
                    continue
                try:
                    for slot in slots:
                        slot.slot(device, message, params)
                except:
                    device.logger.exception('error in slot "%s"', slot)
                slot = slots = None
        finally:
            self.emit('call', {'*': ['slotInstanceGone']}, self.deviceId, info)

    def decodeMessage(self, message):
        hash = Hash.decode(message.data, "Bin")
        params = []
        for i in count(1):
            try:
                params.append(hash['a{}'.format(i)])
            except KeyError:
                break
        replyFrom = message.properties.get('replyFrom')
        if replyFrom is not None:
            f = self.repliers.get(replyFrom.decode("ascii"))
            if f is not None:
                f.set_result(params)
            return {}, None

        slots = (message.properties['slotFunctions'][1:-1]).decode(
            "utf8").split('||')
        return {k: v.split(",") for k, v in (s.split(":") for s in slots)}, \
                params


class Client(object):
    def onChanged(self, hash):
        for k, v in hash.iteritems():
            a = getattr(type(self), k, None)
            if a is not None:
                self.__dict__[a] = v
                for f in self._futures.get(a, []):
                    f.set_result(v)

    def setValue(self, attr, value):
        self._device._ss.emit("call", {deviceId: ["slotReconfigure"]},
                              Hash(attr.key, value))


class EventLoop(SelectorEventLoop):
    def __init__(self):
        super().__init__()
        self.connection = None
        self.changedFuture = Future(loop=self)  # call if some property changes
        self.set_default_executor(ThreadPoolExecutor(10000))

    def getBroker(self, deviceId, classId):
        if self.connection is None:
            p = openmq.Properties()
            p["MQBrokerHostName"] = "exfl-broker.desy.de"
            p["MQBrokerHostPort"] = 7777
            p["MQConnectionType"] = "TCP"
            p["MQPingInterval"] = 20
            p["MQSSLIsHostTrusted"] = True
            p["MQAckOnProduce"] = False
            p["MQAckTimeout"] = 0

            self.connection = openmq.Connection(p, "guest", "guest")
            self.connection.start()

        return Broker(self, deviceId, classId)

    @coroutine
    def get_device(self, deviceId):
        f = self.device.slotSchemaUpdated[self.deviceId]
        self.device._ss.emit("call", {self.deviceId: ["slotGetSchema"]}, False)
        schema = yield from f
        f = self.device.slotChanged[self.deviceId]
        self.device._ss.emit("call", {self.deviceId: ["slotGetConfiguration"]})
        self.device._ss.connect(self.deviceId, "signalChanged",
                                self.device.slotChanged)
        hash = yield from f

        dict = {}
        for k, v, a in self.schema.hash.iterall():
            if a["nodeType"] == 0:
                d = Type.fromname[a["valueType"]]()
                d.key = k
                dict[k] = d
            elif a["nodeType"] == 1 and a["displayType"] == "Slot":
                s = Slot()
                s.method = lambda self, name=k: self._device._ss.emit(
                    "call", {self.deviceId: [name]})
                dict[k] = s
        cls = type(self.schema.name.encode("ascii"), (Client,), dict)
        o = cls()
        o._device = self.device
        o._futures = {}
        self.device.clients[self.deviceId] = o
        o.onChanged(hash)
        return o

    @coroutine
    def getValueFuture(self, device, attr):
        return device._futures.setdefault(attr, Future(loop=self))

    @coroutine
    def waitForChanges(self):
        yield from self.changedFuture

    def close(self):
        for t in Task.all_tasks(self):
            t.cancel()
        self._ready.extend(self._scheduled)
        self._scheduled.clear()
        while self._ready:
            self._run_once()
        self.connection.close()
        super().close()
