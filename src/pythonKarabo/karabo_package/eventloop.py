from __future__ import absolute_import, unicode_literals
from karabo.hashtypes import Type, Slot
from karabo.hash import Hash, BinaryWriter
from karabo import openmq

from asyncio import (coroutine, Future, get_event_loop, set_event_loop,
                     SelectorEventLoop, Task)
from copy import copy
from concurrent.futures import ThreadPoolExecutor
from itertools import chain
import sys


class Broker:
    def __init__(self, loop, deviceId, classId):
        self.loop = loop
        self.connection = loop.connection
        self.session = openmq.Session(self.connection, False, 1, 0)
        self.destination = openmq.Destination(self.session, "teichman", 1)
        self.producer = openmq.Producer(self.session, self.destination)
        self.deviceId = deviceId
        self.classId = classId

    def call(self, signal, targets, reply, args):
        hash = Hash()
        for i, a in enumerate(args):
            hash['a{}'.format(i + 1)] = a
        p = openmq.Properties()
        p['signalFunction'] = signal
        p['signalInstanceId'] = self.deviceId
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
        p['__format'] = 'Bin'
        writer = BinaryWriter()
        m = openmq.BytesMessage()
        m.data = writer.write(hash)
        m.properties = p
        self.producer.send(m, 1, 4, 1000)

    def emit(self, signal, targets, *args):
        self.call(signal, targets, None, args)

    def reply(self, replyTo, whom, *args):
        hash = Hash()
        for i, a in enumerate(args):
            hash['a{}'.format(i + 1)] = a
        p = openmq.Properties()
        p['replyFrom'] = replyTo
        p['signalInstanceId'] = self.deviceId
        p['signalFunction'] = "__reply__"
        p['slotInstanceIds'] = b'|' + whom + b'|'
        p['__format'] = 'Bin'
        writer = BinaryWriter()
        m = openmq.BytesMessage()
        m.data = writer.write(hash)
        m.properties = p
        self.producer.send(m, 1, 4, 1000)

    def replyNoWait(self, replyInstanceIds, replyFunctions, *args):
        hash = Hash()
        for i, a in enumerate(args):
            hash['a{}'.format(i + 1)] = a
        p = openmq.Properties()
        p['signalInstanceId'] = self.deviceId
        p['signalFunction'] = "__replyNoWait__"
        p['slotInstanceIds'] = replyInstanceIds
        p['slotFunctions'] = replyFunctions
        p['__format'] = 'Bin'
        writer = BinaryWriter()
        m = openmq.BytesMessage()
        m.data = writer.write(hash)
        m.properties = p
        self.producer.send(m, 1, 4, 1000)

    def connect(self, deviceId, signal, slot):
        self.emit("call", {deviceId: ["slotConnectToSignal"]}, signal,
                  slot.__self__.deviceId, slot.__name__, 0)

    def registerSlot(self, x):
        """ legacy code. slots don't need to be registered """
        return


class Client(object):
    def onChanged(self, hash):
        for k, v in hash.iteritems():
            a = getattr(type(self), k, None)
            if a is not None:
                self.__dict__[a] = v
                for f in self._futures.get(a, []):
                    f.set_result(v)

    def setValue(self, attr, value):
        self._device._ss.emit("call", ["slotReconfigure"], [self.deviceId],
                              Hash(attr.key, value))


class EventLoop(SelectorEventLoop):
    def __init__(self):
        super().__init__()
        self.connection = None
        self.changedFuture = Future()  # call if some property changes
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
        self.device._ss.emit("call", ["slotGetSchema"], [self.deviceId], False)
        schema = yield from f
        f = self.device.slotChanged[self.deviceId]
        self.device._ss.emit("call", ["slotGetConfiguration"], [self.deviceId])
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
                    "call", [name], [self.deviceId])
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
