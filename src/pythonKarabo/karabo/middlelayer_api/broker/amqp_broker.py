# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.
from __future__ import absolute_import, unicode_literals

import asyncio
import inspect
import logging
import os
import socket
import time
import traceback
import weakref
from asyncio import (
    CancelledError, Future, Lock, TimeoutError, ensure_future, gather, sleep,
    wait_for)
from contextlib import AsyncExitStack
from functools import partial, wraps
from itertools import count

import aio_pika

from karabo.native import (
    Hash, KaraboError, decodeBinary, decodeBinaryPos, encodeBinary)

from .amqp_cluster import connect_cluster
from .base import Broker


class AmqpBroker(Broker):
    global_lock = Lock()
    connection = None

    def __init__(self, loop, deviceId, classId, broadcast=True):
        super(AmqpBroker, self).__init__(True)
        self.domain = loop.topic
        self.loop = loop
        self.deviceId = deviceId
        self.classId = classId
        self.broadcast = broadcast
        self.repliers = {}
        self.tasks = set()
        self.logger = logging.getLogger(deviceId)
        self.info = None
        self.slots = {}
        self.exitStack = AsyncExitStack()
        # AMQP specific bookkeeping ...
        self.subscriptions = set()  # registered tuple: exchange, routing key
        self.channel = None         # channel for communication
        self.exchanges = {}         # registered exchanges
        self.queue = None           # main queue
        self.consumer_tag = None    # tag returned by consume method
        # Connection birth time representing device ID incarnation.
        self.timestamp = time.time() * 1000000 // 1000      # float
        self.logproc = None
        self.future = None
        self.heartbeatTask = None
        self.lastPublishTask = None
        self.subscrLock = Lock()

    async def subscribe_default(self):
        """Subscribe to 'default' exchanges to allow a communication
        through the broker:
            -------------
            exchange = <domain>.global_slots
            routing_key = ''
            queue = <deviceId>
            -------------
            exchange = <domain>.slots
            routing_key = <deviceId>
            queue = <deviceId>
        """
        self.queue = await self.channel.declare_queue(exclusive=True)
        # create main exchange : <domain>.slots
        exchange = self.domain + ".slots"
        binding_key = self.deviceId
        exch = await self.channel.declare_exchange(
                exchange, aio_pika.ExchangeType.TOPIC)
        self.exchanges[exchange] = exch   # aio_pika.exchange.Exchange instance
        # bind to the queue
        await self.queue.bind(exch, routing_key=binding_key)
        async with self.subscrLock:
            self.subscriptions.add((exch, binding_key))

        if self.broadcast:
            # declare exchange
            exchange = self.domain + ".global_slots"
            binding_key = ""
            exch = await self.channel.declare_exchange(
                    exchange, aio_pika.ExchangeType.TOPIC)
            self.exchanges[exchange] = exch
            await self.queue.bind(exch, routing_key=binding_key)
            async with self.subscrLock:
                self.subscriptions.add((exch, binding_key))

        exchange = self.domain + ".signals"
        self.exchanges[exchange] = await self.channel.declare_exchange(
                exchange, aio_pika.ExchangeType.TOPIC)
        exchange = self.domain + ".log"
        self.exchanges[exchange] = await self.channel.declare_exchange(
                exchange, aio_pika.ExchangeType.TOPIC)

    async def publish(self, exchange, routing_key, message):
        while True:
            try:
                exch = self.exchanges.get(exchange, None)
                if exch is None:
                    exch = await self.channel.declare_exchange(
                            exchange, aio_pika.ExchangeType.TOPIC)
                    self.exchanges[exchange] = exch
                await exch.publish(message, routing_key)
                break
            except BaseException:
                # Channel closed ... wait forever ...
                await sleep(0.1)

    def send(self, exchange, routing_key, header, args):
        if self.loop.is_closed() or not self.loop.is_running():
            return
        body = Hash()
        for i, a in enumerate(args):
            body['a{}'.format(i + 1)] = a
        header['signalInstanceId'] = self.deviceId
        header['__format'] = 'Bin'
        header['producerTimestamp'] = self.timestamp
        bindata = b''.join([encodeBinary(header), encodeBinary(body)])
        m = aio_pika.Message(bindata)
        # publish is awaitable
        self.lastPublishTask = self.loop.create_task(
                self.publish(exchange, routing_key, m))

    def heartbeat(self, interval):
        exchange = self.domain + ".signals"
        routing_key = self.deviceId + ".signalHeartbeat"
        header = Hash("signalFunction", "signalHeartbeat")
        # Note: C++ adds
        header["signalInstanceId"] = self.deviceId  # redundant and unused
        header["slotInstanceIds"] = "__none__"      # unused
        header["slotFunctions"] = "__none__"        # unused
        header["__format"] = "Bin"
        body = Hash()
        body["a1"] = self.deviceId
        body["a2"] = interval
        body["a3"] = self.info
        bindata = b''.join([encodeBinary(header), encodeBinary(body)])
        m = aio_pika.Message(bindata)
        # publish is awaitable
        self.loop.call_soon_threadsafe(
                self.loop.create_task, self.publish(exchange, routing_key, m))

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
            except CancelledError:
                pass
            finally:
                self.emit('call', {'*': ['slotInstanceGone']},
                          self.deviceId, self.info)

        self.heartbeatTask = ensure_future(heartbeat())

    def call(self, signal, targets, reply, args):
        if not targets:
            return
        p = Hash()
        p['signalFunction'] = signal
        slotInstanceIds = (
            '|' + '||'.join(t for t in targets) + '|')
        p['slotInstanceIds'] = slotInstanceIds
        funcs = ("{}:{}".format(k, ",".join(v)) for k, v in targets.items())
        p['slotFunctions'] = ('|' + '||'.join(funcs) + '|')
        if reply is not None:
            p['replyTo'] = reply
        p['hostname'] = socket.gethostname()
        p['classId'] = self.classId
        # AMQP specific follows ...
        slotInstanceId = slotInstanceIds.strip("|")
        if (signal == "__request__" or signal == "__replyNoWait__"
                or signal == "__reply__" or signal == "__replyNoWait"):
            exchange = self.domain + ".slots"
            routing_key = slotInstanceId
        elif signal == "call" or signal == "__call__":
            if slotInstanceId == '*':
                exchange = self.domain + ".global_slots"
                routing_key = ""
            else:
                exchange = self.domain + ".slots"
                routing_key = slotInstanceId
        else:
            exchange = self.domain + ".signals"
            routing_key = self.deviceId + '.' + signal

        self.send(exchange, routing_key, p, args)

    async def request(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex()[4:-4])
        self.call("call", {device: [target]}, reply, args)
        future = Future(loop=self.loop)
        self.repliers[reply] = future
        future.add_done_callback(lambda _: self.repliers.pop(reply))
        return (await future)

    def log(self, message):
        exchange = self.domain + ".log"
        routing_key = ""
        header = Hash("target", "log")
        body = Hash("messages", [message])
        bindata = b''.join([encodeBinary(header), encodeBinary(body)])
        m = aio_pika.Message(bindata)
        self.loop.call_soon_threadsafe(
                self.loop.create_task, self.publish(exchange, routing_key, m))

    def emit(self, signal, targets, *args):
        self.call(signal, targets, None, args)

    def reply(self, message, reply, error=False):
        header = message.get('header')
        sender = header['signalInstanceId']

        if not isinstance(reply, tuple):
            reply = reply,

        if 'replyTo' in header:
            replyTo = header['replyTo']
            p = Hash()
            p['replyFrom'] = replyTo
            p['signalFunction'] = "__reply__"
            p['slotInstanceIds'] = '|' + sender + '|'
            p['error'] = error
            exchange = self.domain + ".slots"
            routing_key = sender
            self.send(exchange, routing_key, p, reply)

        if 'replyInstanceIds' in header:
            replyId = header['replyInstanceIds']
            p = Hash()
            p['signalFunction'] = "__replyNoWait__"
            p['slotInstanceIds'] = replyId
            p['slotFunctions'] = header['replyFunctions']
            p['error'] = error
            dest = replyId.strip('|')
            exchange = self.domain + ".slots"
            routing_key = dest
            self.send(exchange, routing_key, p, reply)

    def replyException(self, message, exception):
        trace = ''.join(traceback.format_exception(
            type(exception), exception, exception.__traceback__))
        self.reply(message, (str(exception), trace), error=True)

    def connect(self, deviceId, signal, slot):
        """This is way of establishing 'karabo signalslot' connection with
        a signal.  In AMQP case this is just creating a task that will
        call `async_connect` that, in turn, may take time because communication
        with RabbitMQ broker is involved.
        """
        self.loop.call_soon_threadsafe(self.loop.create_task,
                                       self.async_connect(deviceId,
                                                          signal, slot))

    async def async_connect(self, deviceId, signal, slot):
        """This is way of establishing karabo connection between local slot and
        remote signal.  In AMQP case this is two steps procedure:
        1. subscription to the topic with topic name built from signal info
        2. sending to the signal side the slot registration message
        NOTE: Optimization: we can connect many signals to the same alot
        at once
        """
        if isinstance(signal, (list, tuple)):
            signals = list(signal)
        else:
            signals = [signal]

        exchange = self.domain + ".signals"
        if exchange not in self.exchanges:
            exch = await self.channel.declare_exchange(
                    exchange, aio_pika.ExchangeType.TOPIC)
            self.exchanges[exchange] = exch
        else:
            exch = self.exchanges[exchange]

        for s in signals:
            binding_key = deviceId + "." + s
            t = (exch, binding_key)
            async with self.subscrLock:
                if t not in self.subscriptions:
                    await self.queue.bind(exch, routing_key=binding_key)
                    self.subscriptions.add(t)

            self.emit("call", {deviceId: ["slotConnectToSignal"]},
                      s, slot.__self__.deviceId, slot.__name__)

    def disconnect(self, deviceId, signal, slot):
        if self.loop.is_closed() or not self.loop.is_running():
            return
        self.loop.call_soon_threadsafe(self.loop.create_task,
                                       self.async_disconnect(deviceId,
                                                             signal, slot))

    async def async_disconnect(self, deviceId, signal, slot):
        signals = []
        if isinstance(signal, (list, tuple)):
            signals = list(signal)
        else:
            signals = [signal]
        if self.loop.is_closed() or not self.loop.is_running():
            self.logger.warning(
                f'Cannot disconnect from signals: {signals}')
            return

        exchange = self.domain + ".signals"
        if exchange not in self.exchanges:
            exch = await self.channel.declare_exchange(
                    exchange, aio_pika.ExchangeType.TOPIC)
            self.exchanges[exchange] = exch
        else:
            exch = self.exchanges[exchange]
        try:
            for s in signals:
                binding_key = deviceId + "." + s
                t = (exch, binding_key)
                async with self.subscrLock:
                    if t in self.subscriptions:
                        self.subscriptions.remove(t)
                        await self.queue.unbind(exch, routing_key=binding_key)

                self.emit("call", {deviceId: ["slotDisconnectFromSignal"]},
                          s, slot.__self__.deviceId, slot.__name__)
        except BaseException:
            self.logger.warning(
                f'Fail to disconnect from signals: {signals}')

    async def async_unsubscribe_all(self):
        async with self.subscrLock:
            for exch, binding_key in self.subscriptions:
                await self.queue.unbind(exch, routing_key=binding_key)
            self.subscriptions = set()

    async def stopHeartbeat(self):
        if self.heartbeatTask is not None:
            if not self.heartbeatTask.done():
                self.heartbeatTask.cancel()
                await self.heartbeatTask
            self.heartbeatTask = None
        task = self.lastPublishTask
        self.lastPublishTask = None
        if task is not None and not task.done():
            await wait_for(task, timeout=1)

    async def _cleanup(self):
        await self.stopHeartbeat()
        self.future.set_result(None)
        if self.logproc is not None:
            self.logproc = None
        if self.consumer_tag is not None:
            await self.queue.cancel(self.consumer_tag)
            self.consumer_tag = None
        await self.async_unsubscribe_all()

    async def on_message(self, device, message: aio_pika.IncomingMessage):
        async with message.process():
            d = device()
            if d is None:
                return
            await self.handleMessage(message.body, d)

    async def handleMessage(self, message, device):
        """Decode message from binary blob
        has valid information to do so...  Otherwise
        simply call handle the message as usual...
        """
        try:
            header, pos = decodeBinaryPos(message)
            islog = header.get('target', '')
            if islog == 'log':
                if self.logproc is not None:
                    self.logproc(message[pos:])
                return
            body = decodeBinary(message[pos:])
            decoded = Hash("header", header, "body", body)
        except BaseException:
            self.logger.exception("Malformed message")
            return
        self._handleMessage(decoded, device)

    def _handleMessage(self, decoded, device):
        try:
            slots, params = self.decodeMessage(decoded)
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
                # call 'coslot.outer(...)' for slot, i.e. reply
                slot.slot(slot, device, name, decoded, params)
        except Exception:
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

    async def consume(self, device):
        self.consumer_tag = await self.queue.consume(
            partial(self.on_message, device))
        # Be under exitStack scope as soon as queue is alive
        self.future = Future()
        await self.future

    async def main(self, device):
        """This is the main loop of a device (SignalSlotable instance)

        A device is running if this coroutine is still running.
        Use `stop_tasks` to stop this main loop."""
        async with self.exitStack:
            device = weakref.ref(device)
            await self.consume(device)

    async def _stop_tasks(self):
        me = asyncio.current_task(loop=None)
        tasks = [t for t in self.tasks if t is not me]
        for t in tasks:
            t.cancel()
        await wait_for(gather(*tasks, return_exceptions=True),
                       timeout=5)

    async def stop_tasks(self):
        """Stop all currently running task

        This marks the end of life of a device.

        Note that the task this coroutine is called from, as an exception,
        is not cancelled. That's the chicken-egg-problem.
        """
        await self._cleanup()
        try:
            await self._stop_tasks()
            return True
        except TimeoutError:
            return False

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

    def decodeMessage(self, hash):
        """Decode a Karabo message

        reply messages are dispatched directly.

        :returns: a dictionary that maps the device id of slots to be called
            to a list of slots to be called on that device
        """
        header = hash.get('header')
        body = hash.get('body')
        params = []
        for i in count(1):
            try:
                params.append(body['a{}'.format(i)])
            except KeyError:
                break
        replyFrom = header.get('replyFrom')
        if replyFrom is not None:
            f = self.repliers.get(replyFrom)
            if f is not None and not f.done():
                if header.get('error', False):
                    exceptTxt = params[0]
                    if len(params) >= 2 and params[1]:
                        exceptTxt += "\nDETAILS: " + params[1]
                    f.set_exception(KaraboError(exceptTxt))
                else:
                    if len(params) == 1:
                        params = params[0]
                    else:
                        params = tuple(params)
                    f.set_result(params)
            return {}, None

        slots = (header['slotFunctions'][1:-1]).split('||')
        return ({k: v.split(",") for k, v in (s.split(":") for s in slots)},
                params)

    def get_property(self, message, prop):
        header = message.get('header')
        if header is None:
            return None
        return header.get(prop)

    @classmethod
    async def _ensure_global_connection(cls):
        async with cls.global_lock:
            if cls.connection is not None:
                return cls.connection
            urls = os.environ.get("KARABO_BROKER",
                                  "amqp://localhost:5672").split(',')
            cls.connection = None
            for url in urls:
                try:
                    # Perform connection
                    cls.connection = await connect_cluster(url)
                    break
                except BaseException:
                    # print(f'While trying node "{url}": {str(e)}')
                    cls.connection = None

            return cls.connection

    async def ensure_connection(self):
        if self.connection is None:
            self.connection = await AmqpBroker._ensure_global_connection()
            if self.connection is None:
                raise RuntimeError("No connection established")
        try:
            # Creating a channel
            self.channel = await self.connection.channel()
            await self.channel.set_qos(prefetch_count=1)
            await self.subscribe_default()
        except BaseException:
            self.channel = None
            raise

    @staticmethod
    def create_connection(hosts, connection):
        return None
