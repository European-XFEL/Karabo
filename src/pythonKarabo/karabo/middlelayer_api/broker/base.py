import inspect
import time
import traceback
import weakref
from abc import ABC, abstractmethod
from asyncio import Future
from contextlib import AsyncExitStack
from functools import wraps


class Broker(ABC):
    def __init__(self, need_subscribe):
        self.needSubscribe = need_subscribe
        self.exitStack = AsyncExitStack()
        self.slots = {}
        self.info = None
        self.repliers = {}
        self.tasks = set()

    def enter_context(self, context):
        """Synchronously enter the exit stack context"""
        return self.exitStack.enter_context(context)

    async def enter_async_context(self, context):
        """Asynchronously enter the exit stack context"""
        return await self.exitStack.enter_async_context(context)

    async def main(self, device):
        """This is the main loop of a device (SignalSlotable instance)

        A device is running if this coroutine is still running.
        Use `stop_tasks` to stop this main loop."""
        async with self.exitStack:
            device = weakref.ref(device)
            await self.consume(device())

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

    def updateInstanceInfo(self, info):
        """update the short information about this instance

        the instance info hash contains a very brief summary of the device.
        It is regularly published, and even lives longer than a device,
        as it is published with the message that the device died."""
        self.info.merge(info)
        self.emit("call", {"*": ["slotInstanceUpdated"]},
                  self.deviceId, self.info)

    def replyException(self, message, exception):
        trace = ''.join(traceback.format_exception(
            type(exception), exception, exception.__traceback__))
        self.reply(message, (str(exception), trace), error=True)

    def emit(self, signal, targets, *args):
        self.call(signal, targets, None, args)

    async def request(self, device, target, *arguments):
        reply = f"{self.deviceId}-{time.monotonic().hex()[4:-4]}"
        self.call("call", {device: [target]}, reply, arguments)
        future = Future(loop=self.loop)
        self.repliers[reply] = future
        future.add_done_callback(lambda _: self.repliers.pop(reply))
        return (await future)

    # ------------------------------------------------------------------------
    # Subclass interface

    async def ensure_connection(self):
        """Ensure a connection to the broker

        This method is used for libraries with async interface
        """

    # ------------------------------------------------------------------------
    # Abstract interface

    @abstractmethod
    def notify_network(self, info):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between.

        :param info: the info Hash that should be published regularly.
        """

    @abstractmethod
    async def consume(self, device):
        """The main consume method called from `main`. Implementation must
        be blocking, as the exit stack is bound to leaving this method"""

    @abstractmethod
    async def consume_beats(self, server):
        """The consume beats method called from the heartbeat mixin

        This is considered when a device server wants to `track` the
        heartbeats.
        """

    @abstractmethod
    async def stop_tasks(self):
        """Stop all currently running task

        This marks the end of life of a device.

        Note that the task this coroutine is called from, as an exception,
        is not cancelled. That's the chicken-egg-problem.
        """

    @abstractmethod
    def call(self, signal, targets, reply, args):
        pass

    @abstractmethod
    def reply(self, message, reply, error=False):
        pass

    @abstractmethod
    def connect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def async_connect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def disconnect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def async_disconnect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def get_property(self, message, prop):
        return None

    @staticmethod
    def create_connection(hosts, connection):
        # Get scheme (protocol) of first URI...
        scheme, _ = hosts[0].split("://")
        if scheme == "tcp":
            from .jms_broker import JmsBroker
            return (JmsBroker.create_connection(hosts, connection),
                    JmsBroker)
        elif scheme == "mqtt":
            from .mqtt_broker import MqttBroker
            return (MqttBroker.create_connection(hosts, connection),
                    MqttBroker)
        elif scheme == "redis":
            from .redis_broker import RedisBroker
            return (RedisBroker.create_connection(hosts, connection),
                    RedisBroker)
        elif scheme == "amqp":
            from .amqp_broker import AmqpBroker
            return (AmqpBroker.create_connection(hosts, connection),
                    AmqpBroker)
        else:
            raise RuntimeError(f"Unsupported protocol {scheme}")
