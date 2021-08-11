# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.
from __future__ import absolute_import, unicode_literals
import asyncio
from asyncio import (
    CancelledError, ensure_future, Future, gather, get_event_loop,
    shield, sleep, wait_for)
from contextlib import closing, AsyncExitStack
import inspect
import logging
import socket
import time
import traceback
import weakref
from functools import wraps
from itertools import count
from . import openmq
from .eventloop import Broker

from karabo.native import KaraboError
from karabo.native import decodeBinary, encodeBinary, Hash

# See C++ karabo/xms/Signal.hh for reasoning about the two minutes...
_MSG_TIME_TO_LIVE = 120000  # in ms - i.e. 2 minutes
_MSG_PRIORITY_HIGH = 4  # never dropped (except if expired)
_MSG_PRIORITY_LOW = 3  # can be dropped in case of congestion


class JmsBroker(Broker):
    def __init__(self, loop, deviceId, classId, broadcast=True):
        super(JmsBroker, self).__init__(False)
        self.loop = loop
        self.connection = loop.connection
        self.session = openmq.Session(self.connection, False, 1, 0)
        self.destination = openmq.Destination(self.session, loop.topic, 1)
        self.producer = openmq.Producer(self.session, self.destination)
        self.hbdestination = openmq.Destination(self.session,
                                                loop.topic + "_beats", 1)
        self.hbproducer = openmq.Producer(self.session, self.hbdestination)
        self.deviceId = deviceId
        self.classId = classId
        self.broadcast = broadcast
        self.repliers = {}
        self.tasks = set()
        self.logger = logging.getLogger(deviceId)
        self.info = None
        self.slots = {}
        self.exitStack = AsyncExitStack()

    def send(self, p, args):
        hash = Hash()
        for i, a in enumerate(args):
            hash['a{}'.format(i + 1)] = a
        m = openmq.BytesMessage()
        m.data = encodeBinary(hash)
        p['signalInstanceId'] = self.deviceId
        p['__format'] = 'Bin'
        m.properties = p
        self.producer.send(m, 1, _MSG_PRIORITY_HIGH, _MSG_TIME_TO_LIVE)

    def heartbeat(self, interval):
        h = Hash()
        h["a1"] = self.deviceId
        h["a2"] = interval
        h["a3"] = self.info
        m = openmq.BytesMessage()
        m.data = encodeBinary(h)
        p = openmq.Properties()
        p["signalFunction"] = "signalHeartbeat"
        # Note: C++ adds
        # p["signalInstanceId"] = self.deviceId # redundant and unused
        # p["slotInstanceIds"] = "__none__" # unused
        # p["slotFunctions"] = "__none__" # unused
        p["__format"] = "Bin"
        m.properties = p
        self.hbproducer.send(m, 1, _MSG_PRIORITY_LOW, _MSG_TIME_TO_LIVE)

    def notify_network(self, info):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between.

        :param info: the info Hash that should be published regularly.
        """
        self.info = info
        self.emit('call', {'*': ['slotInstanceNew']},
                  self.deviceId, self.info)

        async def heartbeat():
            try:
                first = True
                while True:
                    # Permanently send heartbeats, but first one not
                    # immediately: Those interested in us just got informed.
                    interval = self.info["heartbeatInterval"]
                    # Protect against any bad interval that causes spinning
                    sleepInterval = abs(interval) if interval != 0 else 10
                    if first:
                        # '//2' protects change of 'tracking factor' close to 1
                        # '+1' protects against 0 if sleepInterval is 1
                        sleepInterval = sleepInterval // 2 + 1
                        first = False
                    await sleep(sleepInterval)
                    self.heartbeat(interval)
            finally:
                self.emit('call', {'*': ['slotInstanceGone']},
                          self.deviceId, self.info)

        ensure_future(heartbeat())

    def call(self, signal, targets, reply, args):
        if not targets:
            return
        p = openmq.Properties()
        p['signalFunction'] = signal
        p['slotInstanceIds'] = (
            '|' + '||'.join(t for t in targets) + '|').encode("utf8")
        funcs = ("{}:{}".format(k, ",".join(v)) for k, v in targets.items())
        p['slotFunctions'] = ('|' + '||'.join(funcs) + '|').encode("utf8")
        if reply is not None:
            p['replyTo'] = reply
        p['hostname'] = socket.gethostname()
        p['classId'] = self.classId
        self.send(p, args)

    async def request(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex()[4:-4])
        self.call("call", {device: [target]}, reply, args)
        future = Future(loop=self.loop)
        self.repliers[reply] = future
        future.add_done_callback(lambda _: self.repliers.pop(reply))
        return (await future)

    def log(self, message):
        p = openmq.Properties()
        p["target"] = "log"
        m = openmq.BytesMessage()
        m.data = encodeBinary(Hash("messages", [message]))
        m.properties = p
        self.producer.send(m, 1, _MSG_PRIORITY_LOW, _MSG_TIME_TO_LIVE)

    def emit(self, signal, targets, *args):
        self.call(signal, targets, None, args)

    def reply(self, message, reply, error=False):
        sender = message.properties['signalInstanceId']

        if not isinstance(reply, tuple):
            reply = reply,

        replyTo = message.properties.get('replyTo')
        if replyTo is not None:
            p = openmq.Properties()
            p['replyFrom'] = replyTo
            p['signalFunction'] = "__reply__"
            p['slotInstanceIds'] = b'|' + sender + b'|'
            p['error'] = error
            self.send(p, reply)

        replyInstanceIds = message.properties.get('replyInstanceIds')
        if replyInstanceIds is not None:
            p = openmq.Properties()
            p['signalFunction'] = "__replyNoWait__"
            p['slotInstanceIds'] = replyInstanceIds
            p['slotFunctions'] = message.properties['replyFunctions']
            p['error'] = error
            self.send(p, reply)

    def replyException(self, message, exception):
        trace = ''.join(traceback.format_exception(
            type(exception), exception, exception.__traceback__))
        self.reply(message, trace, error=True)

    def connect(self, deviceId, signals, slot):
        """This is an interface for establishing connection netween local slot
        and remote signal.  In JMS case we simply send a message in
        fire-and-forget style to the signal side to register a slot
        NOTE: In case of many signals we send multiple messages (same slot)
        """
        if not isinstance(signals, (list, tuple)):
            signals = [signals]

        for s in signals:
            self.emit("call", {deviceId: ["slotConnectToSignal"]}, s,
                      slot.__self__.deviceId, slot.__name__)

    async def async_connect(self, deviceId, signal, slot):
        """Asynchronous signalslot connection in case JMS broker uses
        'connect' interface
        """
        self.connect(deviceId, signal, slot)

    def disconnect(self, deviceId, signals, slot):
        if not isinstance(signals, (list, tuple)):
            signals = [signals]

        for s in signals:
            self.emit("call", {deviceId: ["slotDisconnectFromSignal"]}, s,
                      slot.__self__.deviceId, slot.__name__)

    async def async_disconnect(self, deviceId, signal, slot):
        self.disconnect(deviceId, signal, slot)

    async def consume(self, device):
        loop = get_event_loop()
        device = weakref.ref(device)
        running = True

        broadcast_mq = ("slotInstanceIds LIKE '%|{0.deviceId}|%' "
                        "OR slotInstanceIds LIKE '%|*|%'".format(self))
        device_mq = "slotInstanceIds LIKE '%|{0.deviceId}|%'".format(self)

        def receiver():
            m_mq = broadcast_mq if self.broadcast else device_mq
            consumer = openmq.Consumer(
                self.session, self.destination, m_mq, False)

            with closing(consumer):
                while running:
                    try:
                        message = consumer.receiveMessage(1000)
                    except openmq.Error as e:
                        # statuses from openmqc/mqerrors.h
                        if e.status == openmq.OPEN_MQ_TIMEOUT:
                            continue
                        elif e.status == openmq.OPEN_MQ_CONCURRENT_ACCESS:
                            # Sometimes this error appears. It seems to be a
                            # race condition within openmqc, but retrying just
                            # helps.
                            loop.call_soon_threadsafe(
                                self.logger.warning,
                                'consumer of instance "%s" had a concurrent '
                                'access',
                                self.deviceId)
                            continue
                        elif e.status == openmq.OPEN_MQ_MSG_DROPPED:
                            loop.call_soon_threadsafe(
                                self.logger.warning,
                                'consumer of instance "%s" dropped messages',
                                self.deviceId)
                            message = e.message
                        else:
                            raise
                    d = device()
                    if d is None:
                        return
                    loop.call_soon_threadsafe(
                        loop.create_task, self.handleMessage(message, d), d)
                    d = None

        task = loop.run_in_executor(None, receiver)
        try:
            await shield(task)
        except CancelledError:
            running = False
            await task
            raise

    async def handleMessage(self, message, device):
        try:
            slots, params = self.decodeMessage(message)
        except BaseException:
            self.logger.exception("Malformed message")
            return
        try:
            callSlots = [(self.slots[s], s)
                         for s in slots.get(self.deviceId, [])]
            if self.broadcast:
                callSlots.extend(
                    [(self.slots[s], s)
                     for s in slots.get("*", []) if s in self.slots])
        except KeyError:
            self.logger.exception("Slot does not exist")
            return
        try:
            for slot, name in callSlots:
                slot.slot(slot, device, name, message, params)
        except BaseException:
            # the slot.slot wrapper should already catch all exceptions
            # all exceptions raised additionally are a bug in Karabo
            self.logger.exception(
                "Internal error while executing slot")

    def register_slot(self, name, slot):
        """register a slot on the device

        :param name: the name of the slot
        :param slot: the slot to be called. If this is a bound method, it is
            assured that no reference to the object holding the method is kept.
        """
        if inspect.ismethod(slot):
            def delete(ref):
                del self.slots[name]

            weakself = weakref.ref(slot.__self__, delete)
            func = slot.__func__

            @wraps(func)
            def wrapper(*args):
                return func(weakself(), *args)

            self.slots[name] = wrapper
        else:
            self.slots[name] = slot

    async def main(self, device):
        """This is the main loop of a device

        A device is running if this coroutine is still running.
        Use `stop_tasks` to stop this main loop."""
        async with self.exitStack:
            # this method should not keep a reference to the device
            # while yielding, otherwise the device cannot be collected
            device = weakref.ref(device)
            await self.consume(device())

    async def stop_tasks(self):
        """Stop all currently running task

        This marks the end of life of a device.

        Note that the task this coroutine is called from, as an exception,
        is not cancelled. That's the chicken-egg-problem.
        """
        me = asyncio.current_task(loop=None)
        tasks = [t for t in self.tasks if t is not me]
        for t in tasks:
            t.cancel()
        # A task is immediately cancelled. We are waiting for the tasks here
        # to be done, which might take forever and thus we are not able to
        # exit here if we are not putting a timeout of a few seconds.
        # We also do not expect a very long procedure to be executed in a
        # cancellation.
        # A typical case which gets stuck is a logging after an exception when
        # the device is shutdown.
        # Hence, this makes sure the device gets killed and thus a server
        # can shutdown by closing the eventloop.
        await wait_for(gather(*tasks, return_exceptions=True),
                       timeout=5)

    def enter_context(self, context):
        return self.exitStack.enter_context(context)

    async def enter_async_context(self, context):
        return await self.exitStack.enter_async_context(context)

    def updateInstanceInfo(self, info):
        """update the short information about this instance

        the instance info hash contains a very brief summary of the device.
        It is regularly published, and even lives longer than a device,
        as it is published with the message that the device died."""
        self.info.merge(info)
        self.emit("call", {"*": ["slotInstanceUpdated"]},
                  self.deviceId, self.info)

    def decodeMessage(self, message):
        """Decode a Karabo message

        reply messages are dispatched directly.

        :returns: a dictionary that maps the device id of slots to be called
            to a list of slots to be called on that device
        """
        hash = decodeBinary(message.data)
        params = []
        for i in count(1):
            try:
                params.append(hash['a{}'.format(i)])
            except KeyError:
                break
        replyFrom = message.properties.get('replyFrom')
        if replyFrom is not None:
            f = self.repliers.get(replyFrom.decode("ascii"))
            if f is not None and not f.done():
                if message.properties.get('error', False):
                    f.set_exception(KaraboError(params[0]))
                else:
                    if len(params) == 1:
                        params = params[0]
                    else:
                        params = tuple(params)
                    f.set_result(params)
            return {}, None

        slots = (message.properties['slotFunctions'][1:-1]).decode(
            "utf8").split('||')
        return ({k: v.split(",") for k, v in (s.split(":") for s in slots)},
                params)

    def get_property(self, message, prop):
        return message.properties[prop].decode('ascii')

    @staticmethod
    def create_connection(hosts, connection):
        if connection is not None:
            return connection
        for hp in hosts:
            protocol, host, port = hp.split(':')
            if protocol != 'tcp':
                raise RuntimeError("All URIs in KARABO_BROKER must"
                                   " contain the same scheme (protocol)")
            host = host[2:]
            p = openmq.Properties()
            p["MQBrokerHostName"] = host.strip()
            p["MQBrokerHostPort"] = int(port)
            p["MQConnectionType"] = "TCP"
            p["MQPingInterval"] = 20
            p["MQSSLIsHostTrusted"] = True
            p["MQAckOnProduce"] = False
            p["MQAckOnAcknowledge"] = False
            p["MQAckTimeout"] = 0
            try:
                connection = openmq.Connection(p, "guest", "guest")
                connection.start()
                return connection
            except BaseException:
                connection = None
        raise RuntimeError(f"No connection can be established for {hosts}")
