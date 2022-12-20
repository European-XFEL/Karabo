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

import aiormq
from aiormq.types import DeliveredMessage

from karabo.native import (
    Hash, KaraboError, decodeBinary, decodeBinaryPos, encodeBinary)

from ..eventloop import EventLoop
from ..synchronization import allCompleted
from .base import Broker


def ensure_running(func):
    """Ensure a running eventloop for `func`"""
    if asyncio.iscoroutinefunction(func):
        async def wrapper(self, *args, **kwargs):
            if self.loop.is_closed() or not self.loop.is_running():
                self.logger.warning(
                    f"Loop not running for func {func.__name__} with "
                    f"arguments {args}.")
                return
            return await func(self, *args, **kwargs)
    else:
        def wrapper(self, *args, **kwargs):
            if self.loop.is_closed() or not self.loop.is_running():
                self.logger.warning(
                    f"Loop not running for func {func.__name__} with "
                    f"arguments {args}.")
                return
            return func(self, *args, **kwargs)
    return wrapper


class AmqpBroker(Broker):
    def __init__(self, loop, deviceId, classId, broadcast=True):
        super(AmqpBroker, self).__init__(True)
        self.domain = loop.topic
        self.loop = loop
        self.connection = None
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
        self.channel = None  # channel for communication
        self.queue = None  # main queue
        self.consumer_tag = None  # tag returned by consume method
        # Connection birth time representing device ID incarnation.
        self.timestamp = time.time() * 1000000 // 1000  # float
        self.future = None
        self.heartbeatTask = None
        self.subscribe_lock = Lock()

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
        declare_ok = await self.channel.queue_declare(exclusive=True)
        self.queue = declare_ok.queue
        # create main exchange : <domain>.slots
        exchange = f"{self.domain}.slots"
        await self.channel.exchange_declare(exchange=exchange,
                                            exchange_type='topic')
        key = self.deviceId
        await self.channel.queue_bind(self.queue, exchange, routing_key=key)
        async with self.subscribe_lock:
            self.subscriptions.add((exchange, key))

        # Globals for instanceNew, Gone ...
        exchange = f"{self.domain}.global_slots"
        await self.channel.exchange_declare(exchange=exchange,
                                            exchange_type='topic')
        key = ''
        await self.channel.queue_bind(self.queue, exchange, routing_key=key)
        async with self.subscribe_lock:
            self.subscriptions.add((exchange, key))

        exchange = f"{self.domain}.signals"
        await self.channel.exchange_declare(exchange=exchange,
                                            exchange_type='topic')

    def publish(self, exch, key, msg):
        # schedule 'basic_publish' call soon on event loop ...
        self.loop.call_soon_threadsafe(
            self.loop.create_task,
            self.channel.basic_publish(msg, routing_key=key, exchange=exch))

    def encode_binary_message(self, header, arguments):
        body = Hash()
        for i, a in enumerate(arguments):
            body['a{}'.format(i + 1)] = a
        header['signalInstanceId'] = self.deviceId
        header['__format'] = 'Bin'
        header['producerTimestamp'] = self.timestamp
        return b''.join([encodeBinary(header), encodeBinary(body)])

    @ensure_running
    def send(self, exchange, routing_key, header, arguments):
        msg = self.encode_binary_message(header, arguments)
        self.publish(exchange, routing_key, msg)  # fire&forget

    async def async_send(self, exch, key, header, arguments):
        msg = self.encode_binary_message(header, arguments)
        await self.channel.basic_publish(msg, routing_key=key, exchange=exch)

    async def heartbeat(self, interval):
        header = Hash("signalFunction", "signalHeartbeat")
        # Note: C++ adds
        header["signalInstanceId"] = self.deviceId  # redundant and unused
        header["slotInstanceIds"] = "__none__"  # unused
        header["slotFunctions"] = "__none__"  # unused
        header["__format"] = "Bin"
        body = Hash()
        body["a1"] = self.deviceId
        body["a2"] = interval
        body["a3"] = self.info
        msg = b''.join([encodeBinary(header), encodeBinary(body)])
        exch = f"{self.domain}.signals"
        key = f"{self.deviceId}.signalHeartbeat"
        await self.channel.basic_publish(msg, routing_key=key, exchange=exch)

    async def notify_network(self, info):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between.

        :param info: the info Hash that should be published regularly.
        """
        self.info = info
        await self.async_emit('call', {'*': ['slotInstanceNew']},
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
                    await self.heartbeat(interval)
            except CancelledError:
                pass
            finally:
                await self.async_emit('call', {'*': ['slotInstanceGone']},
                                      self.deviceId, self.info)

        self.heartbeatTask = ensure_future(heartbeat())

    def build_arguments(self, signal, targets, reply):
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
            name = f"{self.domain}.slots"
            routing_key = slotInstanceId
        elif signal == "call" or signal == "__call__":
            if slotInstanceId == '*':
                name = f"{self.domain}.global_slots"
                routing_key = ""
            else:
                name = f"{self.domain}.slots"
                routing_key = slotInstanceId
        else:
            name = f"{self.domain}.signals"
            routing_key = self.deviceId + '.' + signal

        return name, routing_key, p

    def call(self, signal, targets, reply, arguments):
        if not targets:
            return
        name, routing_key, p = self.build_arguments(signal, targets, reply)
        self.send(name, routing_key, p, arguments)

    async def async_call(self, signal, targets, reply, arguments):
        if not targets:
            return
        name, routing_key, p = self.build_arguments(signal, targets, reply)
        await self.async_send(name, routing_key, p, arguments)

    async def request(self, device, target, *arguments):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex()[4:-4])
        self.call("call", {device: [target]}, reply, arguments)
        future = Future(loop=self.loop)
        self.repliers[reply] = future
        future.add_done_callback(lambda _: self.repliers.pop(reply))
        return (await future)

    def emit(self, signal, targets, *arguments):
        self.call(signal, targets, None, arguments)

    async def async_emit(self, signal, targets, *arguments):
        await self.async_call(signal, targets, None, arguments)

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
            name = f'{self.domain}.slots'
            routing_key = sender
            self.send(name, routing_key, p, reply)

        if 'replyInstanceIds' in header:
            replyId = header['replyInstanceIds']
            p = Hash()
            p['signalFunction'] = "__replyNoWait__"
            p['slotInstanceIds'] = replyId
            p['slotFunctions'] = header['replyFunctions']
            p['error'] = error
            dest = replyId.strip('|')
            name = f'{self.domain}.slots'
            routing_key = dest
            self.send(name, routing_key, p, reply)

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
        self.loop.call_soon_threadsafe(
            self.loop.create_task, self.async_connect(deviceId, signal, slot))

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

        exchange = f"{self.domain}.signals"
        for s in signals:
            key = f"{deviceId}.{s}"
            subscription = (exchange, key)
            async with self.subscribe_lock:
                if subscription not in self.subscriptions:
                    await self.channel.queue_bind(
                        queue=self.queue, exchange=exchange, routing_key=key)
                    self.subscriptions.add(subscription)

            await self.async_emit("call", {deviceId: ["slotConnectToSignal"]},
                                  s, slot.__self__.deviceId, slot.__name__)

    @ensure_running
    def disconnect(self, devId, signal, slot):
        self.loop.call_soon_threadsafe(
            self.loop.create_task, self.async_disconnect(devId, signal, slot))

    @ensure_running
    async def async_disconnect(self, deviceId, signal, slot):
        signals = []
        if isinstance(signal, (list, tuple)):
            signals = list(signal)
        else:
            signals = [signal]

        exchange = f"{self.domain}.signals"
        try:
            for s in signals:
                key = f"{deviceId}.{s}"
                subscription = (exchange, key)
                async with self.subscribe_lock:
                    if subscription in self.subscriptions:
                        self.subscriptions.remove(subscription)
                        await self.channel.queue_unbind(queue=self.queue,
                                                        exchange=exchange,
                                                        routing_key=key)

                await self.async_emit("call",
                                      {deviceId: ["slotDisconnectFromSignal"]},
                                      s, slot.__self__.deviceId, slot.__name__)
        except BaseException:
            self.logger.warning(
                f'Fail to disconnect from signals: {signals}')

    async def async_unsubscribe_all(self):
        async with self.subscribe_lock:
            tasks = [
                asyncio.shield(self.channel.queue_unbind(
                    queue=self.queue, exchange=exchange, routing_key=key)
                ) for exchange, key in self.subscriptions]
            if tasks:
                await allCompleted(*tasks, cancel_pending=False, timeout=5)
            self.subscriptions = set()

    async def handleMessage(self, message, device):
        """Decode message from binary blob
        has valid information to do so...  Otherwise
        simply call handle the message as usual...
        """
        try:
            header, pos = decodeBinaryPos(message)
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

    async def stopHeartbeat(self):
        if self.heartbeatTask is not None:
            if not self.heartbeatTask.done():
                self.heartbeatTask.cancel()
                await self.heartbeatTask
            self.heartbeatTask = None

    async def _cleanup(self):
        await self.stopHeartbeat()
        if self.future is not None:
            self.future.set_result(None)
        if self.consumer_tag is not None:
            await self.channel.basic_cancel(self.consumer_tag)
            self.consumer_tag = None
        await self.async_unsubscribe_all()

    async def on_message(self, device, message: DeliveredMessage):
        d = device()
        if d is not None:
            await self.handleMessage(message.body, d)
        await message.channel.basic_ack(message.delivery.delivery_tag)

    async def consume(self, device):
        consume_ok = await self.channel.basic_consume(
            self.queue, partial(self.on_message, device))
        self.consumer_tag = consume_ok.consumer_tag
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
        connection = EventLoop.global_loop.connection
        if connection and connection.is_opened:
            return connection
        urls = os.environ.get("KARABO_BROKER",
                              "amqp://localhost:5672").split(',')
        for url in urls:
            try:
                # Perform connection
                connection = await aiormq.connect(url)
                EventLoop.global_loop.connection = connection
                break
            except BaseException as e:
                print(f'While trying node "{url}": {str(e)}')
                connection = None
        return connection

    async def ensure_connection(self):
        if self.connection is None:
            self.connection = await asyncio.shield(
                AmqpBroker._ensure_global_connection())
            if self.connection is None:
                raise RuntimeError("No connection established")
        try:
            # Creating a channel
            self.channel = await self.connection.channel()
            await self.channel.basic_qos(prefetch_count=1)
            await self.subscribe_default()
        except BaseException:
            self.channel = None
            raise

    @staticmethod
    def create_connection(hosts, connection):
        return connection
