# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import asyncio
import logging
import os
import socket
import time
import weakref
from asyncio import (
    CancelledError, Lock, TimeoutError, ensure_future, gather, sleep, wait_for)
from functools import partial
from itertools import count

import aiormq
import numpy
from aiormq.base import TaskWrapper

from karabo.native import (
    Hash, KaraboError, decodeBinary, decodeBinaryPos, encodeBinary)

from ..eventloop import EventLoop
from ..utils import suppress
from .base import Broker

_OVERFLOW_POLICY = "drop-head"  # Drop oldest messages
_TIME_TO_LIVE = 120_000  # 120 seconds expiry time [ms]
_BEATS_QSIZE = 12_000
_CONSUME_QSIZE = 100_000


class KaraboWrapper(TaskWrapper):
    """The purpose of this class is to flag external tasks"""

    def __init__(self, task) -> None:
        super().__init__(task)
        task.shielded = True


aiormq.base.TaskWrapper = KaraboWrapper


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
        super().__init__(True)
        self.domain = loop.topic
        self.loop = loop
        self.connection = None
        self.deviceId = deviceId
        self.classId = classId
        self.brokerId = None
        # Interest in receiving broadcasts
        self.broadcast = broadcast
        self.logger = logging.getLogger(deviceId)
        # AMQP specific bookkeeping ...
        self.subscriptions = set()  # registered tuple: exchange, routing key
        self.channel = None  # channel for communication
        self.queue = None  # main queue
        self.consumer_tag = None  # tag returned by consume method
        self.heartbeat_queue = None
        self.heartbeat_consumer_tag = None
        self.exit_event = asyncio.Event()
        self.heartbeat_task = None
        self.subscribe_lock = Lock()
        # Flag to indicate when a channel is about to be closed
        self.shutdown_channel = False

    async def subscribe_default(self):
        """Subscribe to 'default' exchanges to allow a communication
        through the broker:
            -------------
            exchange = <domain>.global_slots
            routing_key = ""
            queue = <deviceId>
            -------------
            exchange = <domain>.slots
            routing_key = <deviceId>
            queue = <deviceId>
        """
        self.brokerId = f"{self.domain}.{self.deviceId}"
        try:
            await self.channel.queue_declare(self.brokerId, passive=True)
            # If no exception raised the the queue name exists already ...
            # To continue  just use generated queue name...
            timestamp = hex(int(time.monotonic() * 1000000000))[2:]
            self.brokerId = f"{self.domain}.{self.deviceId}:{timestamp}"
        except aiormq.exceptions.ChannelNotFoundEntity:
            # Exception raised since the queue not found on the broker
            # The channel is not valid anymore, so create the new one
            self.channel = await self.connection.channel(
                publisher_confirms=False)

        arguments = {
            "x-max-length": _CONSUME_QSIZE,
            "x-overflow": _OVERFLOW_POLICY,
            "x-message-ttl": _TIME_TO_LIVE}
        declare_ok = await self.channel.queue_declare(
            self.brokerId, auto_delete=True, arguments=arguments)
        # The received queue name (either input or generated) is ...
        self.queue = declare_ok.queue
        # create main exchange : <domain>.slots
        exchange = f"{self.domain}.slots"
        await self.channel.exchange_declare(exchange=exchange,
                                            exchange_type="topic")
        key = self.deviceId
        await self.channel.queue_bind(self.queue, exchange, routing_key=key)
        async with self.subscribe_lock:
            self.subscriptions.add((exchange, key))

        # Globals for instanceNew, Gone ...
        exchange = f"{self.domain}.global_slots"
        await self.channel.exchange_declare(exchange=exchange,
                                            exchange_type="topic")
        # Only if interested, we subscribe to broadcasts messages,
        # but still declare the exchange to publish
        if self.broadcast:
            key = ""
            await self.channel.queue_bind(self.queue, exchange,
                                          routing_key=key)
            async with self.subscribe_lock:
                self.subscriptions.add((exchange, key))

        exchange = f"{self.domain}.signals"
        await self.channel.exchange_declare(exchange=exchange,
                                            exchange_type="topic")

    def encode_binary_message(self, header, arguments):
        body = Hash()
        for i, a in enumerate(arguments):
            body[f"a{i + 1}"] = a
        # Add timestamp (epoch in ms) when message is sent
        header["MQTimestamp"] = numpy.int64(time.time() * 1000)
        header["signalInstanceId"] = self.deviceId
        return encodeBinary(header) + encodeBinary(body)

    @ensure_running
    def send(self, exch, key, header, arguments):
        msg = self.encode_binary_message(header, arguments)
        ensure_future(self.channel.basic_publish(
            msg, routing_key=key, exchange=exch))

    async def async_send(self, exch, key, header, arguments):
        msg = self.encode_binary_message(header, arguments)
        await self.channel.basic_publish(msg, routing_key=key, exchange=exch)

    async def heartbeat(self, interval, info):
        header = Hash("signalFunction", "signalHeartbeat")
        header["signalInstanceId"] = self.deviceId
        body = Hash()
        body["a1"] = self.deviceId
        body["a2"] = interval
        body["a3"] = info
        msg = encodeBinary(header) + encodeBinary(body)
        exch = f"{self.domain}.signals"
        key = f"{self.deviceId}.signalHeartbeat"
        await self.channel.basic_publish(msg, routing_key=key, exchange=exch)

    async def notify_network(self, info):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between.

        :param info: the info Hash that should be published regularly.
        """
        self.info = info

        interval = self.info["heartbeatInterval"]
        heartbeat_info = Hash("type", info["type"],
                              "heartbeatInterval", interval)
        await self.async_emit("call", {"*": ["slotInstanceNew"]},
                              self.deviceId, self.info)

        async def heartbeat():
            try:
                while True:
                    await sleep(interval)
                    await self.heartbeat(interval, heartbeat_info)
            except CancelledError:
                pass
            finally:
                await self.async_emit("call", {"*": ["slotInstanceGone"]},
                                      self.deviceId, self.info)

        self.heartbeat_task = ensure_future(heartbeat())

    def build_arguments(self, signal, targets, reply):
        p = Hash()
        p["signalFunction"] = signal
        slotInstanceIds = "||".join(targets)
        p["slotInstanceIds"] = f"|{slotInstanceIds}|"
        funcs = "||".join(f"{k}:{','.join(v)}" for k, v in targets.items())
        p["slotFunctions"] = f"|{funcs}|"
        if reply is not None:
            p["replyTo"] = reply
        p["hostname"] = socket.gethostname()
        p["classId"] = self.classId
        # AMQP specific follows ...
        if signal in {"__replyNoWait__", "__reply__"}:
            name = f"{self.domain}.slots"
            routing_key = slotInstanceIds
        elif signal == "call":
            if slotInstanceIds == "*":
                name = f"{self.domain}.global_slots"
                routing_key = ""
            else:
                name = f"{self.domain}.slots"
                routing_key = slotInstanceIds
        else:
            name = f"{self.domain}.signals"
            routing_key = f"{self.deviceId}.{signal}"

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

    async def async_emit(self, signal, targets, *arguments):
        await self.async_call(signal, targets, None, arguments)

    def reply(self, message, reply, error=False):
        header = message["header"]
        sender = header["signalInstanceId"]

        if not isinstance(reply, tuple):
            reply = reply,

        if replyTo := header.get("replyTo"):
            p = Hash()
            p["replyFrom"] = replyTo
            p["signalFunction"] = "__reply__"
            p["slotInstanceIds"] = f"|{sender}|"
            p["error"] = error
            name = f"{self.domain}.slots"
            routing_key = sender
            self.send(name, routing_key, p, reply)

        if replyId := header.get("replyInstanceIds"):
            p = Hash()
            p["signalFunction"] = "__replyNoWait__"
            p["slotInstanceIds"] = replyId
            p["slotFunctions"] = header["replyFunctions"]
            p["error"] = error
            dest = replyId.strip("|")
            name = f"{self.domain}.slots"
            routing_key = dest
            self.send(name, routing_key, p, reply)

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

        futures = [sleep(0)]
        async with self.subscribe_lock:
            for s in signals:
                key = f"{deviceId}.{s}"
                subscription = (exchange, key)
                if subscription not in self.subscriptions:
                    futures.append(self.channel.queue_bind(
                        queue=self.queue, exchange=exchange, routing_key=key))
                    self.subscriptions.add(subscription)

                futures.append(
                    self.async_emit(
                        "call", {deviceId: ["slotConnectToSignal"]},
                        s, slot.__self__.deviceId, slot.__name__))

        await gather(*futures, return_exceptions=True)

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
            futures = [sleep(0)]
            async with self.subscribe_lock:
                for s in signals:
                    key = f"{deviceId}.{s}"
                    subscription = (exchange, key)
                    if subscription in self.subscriptions:
                        self.subscriptions.remove(subscription)

                        futures.append(self.channel.queue_unbind(
                            queue=self.queue, exchange=exchange,
                            routing_key=key))
                    futures.append(self.async_emit(
                        "call", {deviceId: ["slotDisconnectFromSignal"]},
                        s, slot.__self__.deviceId, slot.__name__))
            await gather(*futures, return_exceptions=True)
        except BaseException:
            self.logger.warning(
                f"Fail to disconnect from signals: {signals}")

    async def async_unsubscribe_all(self):
        futures = [sleep(0)]
        async with self.subscribe_lock:
            futures.extend([
                asyncio.shield(self.channel.queue_unbind(
                    queue=self.queue, exchange=exchange, routing_key=key)
                ) for exchange, key in self.subscriptions])
            self.subscriptions = set()
        await gather(*futures, return_exceptions=True)

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
        except KeyError as e:
            text = f"Slot does not exist: {e}"
            self.logger.error(text)
            exc = KaraboError(text).with_traceback(e.__traceback__)
            self.replyException(decoded, exc)
            return
        try:
            for slot, name in callSlots:
                # call "coslot.outer(...)" for slot, i.e. reply
                slot.slot(slot, device, name, decoded, params)
        except Exception as e:
            # the slot.slot wrapper should already catch all exceptions
            # all exceptions raised additionally are a bug in Karabo
            text = f"Internal error while executing slot: {e}"
            self.logger.error(text)
            exc = KaraboError(text).with_traceback(e.__traceback__)
            self.replyException(decoded, exc)

    async def _stop_heartbeat(self):
        if self.heartbeat_task is not None:
            if not self.heartbeat_task.done():
                self.heartbeat_task.cancel()
                await self.heartbeat_task
            self.heartbeat_task = None

    async def stop_tasks(self):
        """Reimplemented method of `Broker`"""
        await self._on_stop_tasks()
        me = asyncio.current_task(loop=None)
        # Services that are listening to broadcasts don't need their
        # channel closed, e.g. clients or servers. This will happen
        # on connection closure
        if not self.broadcast:
            me.add_done_callback(self.close_channel)

        tasks = [t for t in self.tasks if t is not me
                 and not getattr(t, "shielded", False)]
        for t in tasks:
            t.cancel()
        try:
            await wait_for(gather(*tasks, return_exceptions=True),
                           timeout=5)
            return True
        except TimeoutError:
            return False

    def close_channel(self, cb):
        """Close channel callback after slotKillDevice"""
        if self.shutdown_channel:
            return
        self.shutdown_channel = True

        async def closing_channel():
            if self.channel is not None:
                await self.channel.close()
                self.channel = None
            self.connection = None

        ensure_future(closing_channel())

    async def _on_stop_tasks(self):
        if self.consumer_tag is not None:
            with suppress(BaseException):
                await self.channel.basic_cancel(self.consumer_tag)
            self.consumer_tag = None
        if self.heartbeat_consumer_tag is not None:
            with suppress(BaseException):
                await self.channel.basic_cancel(self.heartbeat_consumer_tag)
            self.heartbeat_consumer_tag = None
        with suppress(BaseException):
            await self.exitStack.aclose()
        self.exit_event.set()
        await self._stop_heartbeat()
        await self.async_unsubscribe_all()

    async def on_message(self, device, message):
        d = device()
        if d is not None:
            await self.handleMessage(message.body, d)

    async def consume(self, device):
        device = weakref.ref(device)
        consume_ok = await self.channel.basic_consume(
            self.queue, partial(self.on_message, device),
            exclusive=True, no_ack=True)
        # no_ack means automatic acknowlegdement
        self.consumer_tag = consume_ok.consumer_tag
        # Be under exitStack scope as soon as queue is alive
        await self.exit_event.wait()

    async def on_heartbeat(self, device, message):
        d = device()
        if d is not None:
            message = message.body
            try:
                _, pos = decodeBinaryPos(message)
                hsh = decodeBinary(message[pos:])
            except BaseException:
                self.logger.exception("Malformed message")
            else:
                instance_id, info = hsh["a1"], hsh["a3"]
                await d.updateHeartBeat(instance_id, info)

    async def consume_beats(self, device):
        """Heartbeat method for the device server"""
        device = weakref.ref(device)
        name = f"{self.brokerId}:beats"
        arguments = {
            "x-max-length": _BEATS_QSIZE,
            "x-overflow": _OVERFLOW_POLICY,
            "x-message-ttl": _TIME_TO_LIVE}
        declare_ok = await self.channel.queue_declare(
            name, auto_delete=True, arguments=arguments)
        self.heartbeat_queue = declare_ok.queue
        consume_ok = await self.channel.basic_consume(
            self.heartbeat_queue, partial(self.on_heartbeat, device),
            no_ack=True)
        self.heartbeat_consumer_tag = consume_ok.consumer_tag

        # Binding and book-keeping!
        exchange = f"{self.domain}.signals"
        key = "*.signalHeartbeat"
        await self.channel.queue_bind(self.heartbeat_queue, exchange,
                                      routing_key=key)
        async with self.subscribe_lock:
            self.subscriptions.add((exchange, key))

    def decodeMessage(self, hash):
        """Decode a Karabo message

        reply messages are dispatched directly.

        :returns: a dictionary that maps the device id of slots to be called
            to a list of slots to be called on that device
        """
        header = hash["header"]
        body = hash["body"]
        params = []
        for i in count(1):
            try:
                params.append(body[f"a{i}"])
            except KeyError:
                break

        replyFrom = header.get("replyFrom")
        if replyFrom is not None:
            f = self.repliers.get(replyFrom)
            if f is not None and not f.done():
                if header.get("error", False):
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

        slots = (header["slotFunctions"][1:-1]).split("||")
        return ({k: v.split(",") for k, v in (s.split(":") for s in slots)},
                params)

    def get_property(self, message, prop):
        header = message.get("header")
        return header.get(prop) if header is not None else None

    async def create_global_connection(self):
        connection = EventLoop.global_loop.connection
        if connection and connection.is_opened:
            return connection
        urls = os.environ.get("KARABO_BROKER",
                              "amqp://localhost:5672").split(",")
        for url in urls:
            try:
                # Perform connection
                connection = await aiormq.connect(url)
                EventLoop.global_loop.connection = connection
                break
            except BaseException as e:
                print(f"While trying node '{url}': {str(e)}")
                connection = None
        return connection

    async def ensure_connection(self):
        if self.connection is None:
            self.connection = await asyncio.shield(
                self.create_global_connection())
        if self.connection is None:
            raise RuntimeError("No connection established")
        # Creating a channel
        self.channel = await self.connection.channel(
            publisher_confirms=False)
        await self.channel.basic_qos(prefetch_count=1)
        await self.subscribe_default()

    @staticmethod
    def create_connection(hosts, connection):
        return connection
