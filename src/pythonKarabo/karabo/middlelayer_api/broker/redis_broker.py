from __future__ import absolute_import, unicode_literals

import asyncio
import inspect
import logging
import os
import socket
import time
import traceback
import uuid
import weakref
from asyncio import (
    CancelledError, Future, Lock, TimeoutError, ensure_future, gather,
    get_event_loop, sleep, wait_for)
from contextlib import AsyncExitStack
from functools import partial, wraps
from itertools import count

import aioredis

from karabo.native import (
    Hash, KaraboError, decodeBinary, decodeBinaryPos, encodeBinary)

from .base import Broker


class RedisMessage:
    topic: bytes
    payload: bytes


class RedisBroker(Broker):
    def __init__(self, loop, deviceId, classId, broadcast=True):
        super(RedisBroker, self).__init__(True)
        self.domain = loop.topic
        self.loop = loop
        self.clientId = str(uuid.uuid4())
        self.deviceId = deviceId
        self.classId = classId
        self.broadcast = broadcast
        self.repliers = {}
        self.tasks = set()
        self.logger = logging.getLogger(deviceId)
        self.info = Hash()
        self.slots = {}
        self.exitStack = AsyncExitStack()
        self.subscriptions = set()
        self.redis = None       # Redis pool
        self.mpsc = None        # Multi-producer single consumer queue
        # Client birth time representing device ID incarnation.
        self.timestamp = time.time() * 1000000 // 1000      # float
        self.logproc = None
        # Every RedisBroker instance has the following 4 dictionaries...
        self.conMap = {}        # consumer order number's map
        self.conTStamp = {}     # consumer timestamp's map ("incarnation")
        self.proMap = {}        # producer order number's map
        self.store = {}         # store for pending messages (re-ordering)
        self.heartbeatTask = None
        self.lastPublishTask = None
        self.subscrLock = Lock()
        self.readerTask = None

    async def subscribe_default(self):
        """Subscribe to 'default' topics to allow a communication
        through the broker:
            <domain>/global_slots
            <domain>/slots/<deviceId>
        """
        # subscribe to all slots of instance
        topic = self.domain + "/slots/" + self.deviceId.replace('/', '|')
        async with self.subscrLock:
            self.subscriptions.add(topic)
            topics = [self.mpsc.channel(topic)]
        # subscribe to all global slots
        if self.broadcast:
            topic_broadcast = self.domain + "/global_slots"
            async with self.subscrLock:
                if topic_broadcast not in self.subscriptions:
                    self.subscriptions.add(topic_broadcast)
                    topics.append(self.mpsc.channel(topic_broadcast))
        await self.redis.subscribe(*topics)

    async def publish(self, topic, message):
        # This wrapper returns coro and needed because
        # self.redis.publish return future
        await self.redis.publish(topic, message)

    def send(self, topic, header, args):
        if self.loop.is_closed() or not self.loop.is_running():
            return
        body = Hash()
        for i, a in enumerate(args):
            body['a{}'.format(i + 1)] = a
        header['signalInstanceId'] = self.deviceId
        header['__format'] = 'Bin'
        header['producerTimestamp'] = self.timestamp
        self.incrementOrderNumbers(topic, header)
        m = b''.join([encodeBinary(header), encodeBinary(body)])
        self.lastPublishTask = self.loop.create_task(self.publish(topic, m))

    def incrementOrderNumbers(self, topic, header):
        if "orderNumbers" in header:
            del header['orderNumbers']
        if header['slotInstanceIds'] == '|*|':
            return
        if header['signalFunction'] == 'signalChanged':
            return
        slots = (header['slotInstanceIds'][1:-1]).split('||')
        orders = []
        for slot in slots:
            if slot not in self.proMap:
                self.proMap[slot] = 0
            self.proMap[slot] += 1
            orders.append(str(self.proMap[slot]))
        header['orderNumbers'] = ','.join(orders)

    async def heartbeat(self, interval):
        topic = (self.domain + "/signals/" + self.deviceId.replace('/', '|')
                 + "/signalHeartbeat")
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
        m = b''.join([encodeBinary(header), encodeBinary(body)])
        await self.redis.publish(topic, m)

    def notify_network(self, info):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between.

        :param info: the info Hash that should be published regularly.
        """
        self.info.merge(info)
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
                    await self.heartbeat(interval)
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
        signal_id = self.deviceId.replace('/', '|')
        slot_id = slotInstanceIds.strip('|').replace('/', '|')
        if (signal == "__request__" or signal == "__replyNoWait__"
                or signal == "__reply__" or signal == "__replyNoWait"):
            topic = self.domain + "/slots/" + slot_id
        elif signal == "call" or signal == "__call__":
            if slot_id == '*':
                topic = self.domain + "/global_slots"
            else:
                topic = self.domain + "/slots/" + slot_id
        else:
            topic = self.domain + "/signals/" + signal_id + '/' + signal
        self.send(topic, p, args)

    async def request(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex()[4:-4])
        self.call("call", {device: [target]}, reply, args)
        future = Future(loop=self.loop)
        self.repliers[reply] = future
        future.add_done_callback(lambda _: self.repliers.pop(reply))
        return (await future)

    def log(self, message):
        topic = self.domain + "/log"
        header = Hash("target", "log")
        body = Hash("messages", [message])
        m = b''.join([encodeBinary(header), encodeBinary(body)])

        async def inner():
            await self.redis.publish(topic, m)
        ensure_future(inner())

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
            topic = self.domain + "/slots/" + sender.replace('/', '|')
            self.send(topic, p, reply)

        if 'replyInstanceIds' in header:
            replyId = header['replyInstanceIds']
            p = Hash()
            p['signalFunction'] = "__replyNoWait__"
            p['slotInstanceIds'] = replyId
            p['slotFunctions'] = header['replyFunctions']
            p['error'] = error
            dest = replyId.strip('|')
            topic = self.domain + "/slots/" + dest.replace('/', '|')
            self.send(topic, p, reply)

    def replyException(self, message, exception):
        trace = ''.join(traceback.format_exception(
            type(exception), exception, exception.__traceback__))
        self.reply(message, (str(exception), trace), error=True)

    def connect(self, deviceId, signal, slot):
        """This is way of establishing "karabo signalslot"connection with
        a signal.  In REDIS case this is just creating a task that will
        call `async_connect` that, in turn, may take time becuase communication
        with REDIS broker is involved.
        """
        self.loop.call_soon_threadsafe(
            self.loop.create_task, self.async_connect(deviceId, signal, slot))

    async def async_connect(self, deviceId, signal, slot):
        """This is way of establishing karabo connection between local slot and
        remote signal.  In REDIS case this is two steps procedure:
        1. subscription to the topic with topic name built from signal info
        2. sending to the signal side the slot registration message
        NOTE: Optimization: we can connect many signals to the same alot
        at once
        """
        if isinstance(signal, (list, tuple)):
            signals = list(signal)
        else:
            signals = [signal]
        topics = []
        for s in signals:
            topic = (self.domain + "/signals/" + deviceId.replace('/', '|')
                     + "/" + s)
            async with self.subscrLock:
                if topic not in self.subscriptions:
                    self.subscriptions.add(topic)
                    topics.append(self.mpsc.channel(topic))
        if topics:
            await self.redis.subscribe(*topics)
        for s in signals:
            self.emit("call", {deviceId: ["slotConnectToSignal"]},
                      s, slot.__self__.deviceId, slot.__name__)

    def disconnect(self, deviceId, signal, slot):
        if self.loop.is_closed() or not self.loop.is_running():
            return
        self.loop.call_soon_threadsafe(
            self.loop.create_task, self.async_disconnect(deviceId,
                                                         signal, slot))

    async def async_disconnect(self, deviceId, signal, slot):
        if self.loop.is_closed() or not self.loop.is_running():
            self.logger.warning(
                f'Cannot disconnect from signals: {signal}')
            return
        signals = []
        if isinstance(signal, (list, tuple)):
            signals = list(signal)
        else:
            signals = [signal]

        topics = []

        for s in signals:
            topic = (self.domain + "/signals/" + deviceId.replace('/', '|')
                     + "/" + s)
            async with self.subscrLock:
                if topic in self.subscriptions:
                    self.subscriptions.remove(topic)
                    topics.append(topic)
        try:
            if topics:
                await self.redis.unsubscribe(*topics)
            for s in signals:
                self.emit("call", {deviceId: ["slotDisconnectFromSignal"]},
                          s, slot.__self__.deviceId, slot.__name__)
        except BaseException:
            self.logger.warning(
                f'Fail to disconnect from signals: {signals}')

    async def async_unsubscribe_all(self):
        async with self.subscrLock:
            await self.redis.unsubscribe(*[t for t in self.subscriptions])
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
            await wait_for(task, timeout=5)

    async def ensure_disconnect(self):
        """Close broker connection"""
        self.redis.close()
        await self.redis.wait_closed()

    async def _cleanup(self):
        await self.async_unsubscribe_all()
        await self.stopHeartbeat()
        if self.logproc is not None:
            self.logproc = None
        if self.readerTask is not None:
            if not self.readerTask.done():
                self.mpsc.stop()
                await wait_for(self.readerTask, timeout=5)
            self.readerTask = None

    async def handleMessage(self, message, device):
        """Check message order first if the header
        has valid information to do so...  Otherwise
        simply call handle the message as usual...
        """
        try:
            topic = message.topic
            header, pos = decodeBinaryPos(message.payload)
            islog = header.get('target', '')
            if islog == 'log':
                if self.logproc is not None:
                    self.logproc(message.payload[pos:])
                return
            body = decodeBinary(message.payload[pos:])
            hash = Hash("header", header, "body", body)
            self.checkOrder(topic, device, hash)
        except BaseException as e:
            self.logger.exception(f"Malformed message: {e}")

    def checkOrder(self, topic, device, hash):
        loop = get_event_loop()
        callback = partial(self._handleMessage, hash, device)
        header = hash.get('header')
        if ('signalInstanceId' not in header
                or 'slotInstanceIds' not in header
                or 'orderNumbers' not in header
                or header['slotInstanceIds'] == '|*|'):
            loop.call_soon(callback)
            return
        src = header.get('signalInstanceId')
        if 'producerTimestamp' not in header:
            self.logger.exception("Message lacks \"producer timestamp\"")
            return
        proTStamp = header['producerTimestamp']
        if src not in self.conMap:
            # Producer was not known before
            self.conMap[src] = 0
            self.conTStamp[src] = 0.0
            self.store[src] = {}
        slots = (header['slotInstanceIds'][1:-1]).split('||')
        numbers = [int(k) for k in header['orderNumbers'].split(',')]
        # Find deviceId in the list of targets (slotInstanceIds) ...
        recvNumber = None
        for ii, slotname in enumerate(slots):
            if self.deviceId == slotname:
                recvNumber = numbers[ii]
                break
        if recvNumber is None:
            return      # subscribed but slot is not registered yet

        if self.conTStamp[src] != proTStamp:
            # producer is restarted
            self.conTStamp[src] = proTStamp
            self.cleanObsolete(src, proTStamp)  # clean old messages
            self.conMap[src] = 0                # synchronize consumer count

        # Expect message received in order: recvNumber == self.conMap[src]+1
        expectedNumber = self.conMap[src] + 1

        if recvNumber < expectedNumber:
            return                          # ignore duplicated message
        elif recvNumber > expectedNumber:
            # out-of-order: add to store of pending messages
            if self.conMap[src] == 0:
                self.conMap[src] = recvNumber  # synchronize consumer
                loop.call_soon(callback)
            else:
                self.store[src][recvNumber] = (proTStamp, callback)
        else:
            # Message received in order
            self.conMap[src] = recvNumber
            loop.call_soon(callback)
        self.handleStore(src)

    def handleStore(self, src):
        # try to sort and resolve pending messages in store...
        self.store[src] = {
            k: self.store[src][k] for k in sorted(self.store[src])
            if not self.reorderedInStore(src, k, self.store[src][k])}

    def reorderedInStore(self, src, recvNumber, value):
        loop = get_event_loop()
        proTStamp, callback = value
        if proTStamp != self.conTStamp[src]:    # other incarnation
            return False    # keep in store for now
        expected = self.conMap[src] + 1
        if recvNumber > expected:
            return False    # keep pending
        # synchronize counter with producer... if not duplicated
        if recvNumber == expected:
            self.conMap[src] = recvNumber
            loop.call_soon(callback)
        return True         # remove resolved entry

    def cleanObsolete(self, src, valid):
        """Keep only entries with 'valid' timestamp
        """
        self.store[src] = {
            k: self.store[src][k] for k in sorted(self.store[src])
            if self.store[src][k][0] == valid}

    def _handleMessage(self, decoded, device):
        try:
            slots, params = self.decodeMessage(decoded)
        except BaseException as be:
            self.logger.exception(f"Malformed message: {be}")
            return
        try:
            callSlots = [(self.slots[s], s)
                         for s in slots.get(self.deviceId, [])]
            if self.broadcast:
                callSlots.extend(
                    [(self.slots[s], s)
                     for s in slots.get("*", []) if s in self.slots])
        except KeyError as ke:
            self.logger.exception(f"Slot does not exist: {ke}")
            return
        try:
            for slot, name in callSlots:
                # call 'coslot.outer(...)' for slot, i.e. reply
                slot.slot(slot, device, name, decoded, params)
        except Exception as e:
            # the slot.slot wrapper should already catch all exceptions
            # all exceptions raised additionally are a bug in Karabo
            self.logger.exception(
                f"Internal error while executing slot: {e}")

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
        mpsc = aioredis.pubsub.Receiver()
        fut = Future()

        async def reader(mpsc):
            try:
                async for channel, msg in mpsc.iter():
                    d = device()
                    if d is None:
                        continue
                    message = RedisMessage()
                    message.topic = channel
                    message.payload = msg
                    self.loop.call_soon_threadsafe(
                            self.loop.create_task,
                            self.handleMessage(message, d),
                            d)
                    d = None
            except CancelledError:
                pass
            except BaseException as e:
                self.logger.exception(f'Exception in reader: {e}')
            finally:
                fut.set_result(None)

        self.readerTask = self.loop.create_task(reader(mpsc), instance=device)
        self.mpsc = mpsc
        await self.subscribe_default()
        # Block here until 'self.mpsc' will not be stopped
        await fut

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
        finally:
            self.loop.call_soon_threadsafe(
                    self.loop.create_task, self.ensure_disconnect())

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

    async def ensure_connection(self):
        urls = os.environ.get("KARABO_BROKER",
                              "redis://localhost:6379").split(',')
        error_attempts = 0
        for url in urls:
            try:
                self.redis = await aioredis.create_redis_pool(url)
                break
            except aioredis.RedisError:
                error_attempts += 1
        if error_attempts == len(urls):
            raise aioredis.RedisError(
                f'Fail to connect to any of KARABO_BROKER="{urls}"')

    @staticmethod
    def create_connection(hosts, connection):
        return None