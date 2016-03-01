from __future__ import absolute_import, unicode_literals

from asyncio import (AbstractEventLoop, CancelledError, coroutine, gather,
                     Future, get_event_loop, Queue, set_event_loop,
                     SelectorEventLoop, sleep, Task, TimeoutError)
from concurrent.futures import ThreadPoolExecutor
from contextlib import ExitStack
import getpass
from itertools import count
import logging
import os
import queue
import socket
import threading
import weakref

from . import openmq
from .hash import Hash


class Broker:
    def __init__(self, loop, deviceId, classId):
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
        self.repliers = {}
        self.tasks = set()
        self.logger = logging.getLogger(deviceId)
        self.info = Hash()
        self.alive = False

    def send(self, p, args):
        hash = Hash()
        for i, a in enumerate(args):
            hash['a{}'.format(i + 1)] = a
        m = openmq.BytesMessage()
        m.data = hash.encode("Bin")
        p['signalInstanceId'] = self.deviceId
        p['__format'] = 'Bin'
        m.properties = p
        self.producer.send(m, 1, 4, 100000)

    def heartbeat(self, interval):
        h = Hash()
        h["a1"] = self.deviceId
        h["a2"] = interval
        h["a3"] = self.info
        m = openmq.BytesMessage()
        m.data = h.encode("Bin")
        p = openmq.Properties()
        p["signalFunction"] = "signalHeartbeat"
        p["__format"] = "Bin"
        m.properties = p
        self.hbproducer.send(m, 1, 4, 100000)

    @coroutine
    def notify_network(self, interval):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between."""
        self.info["heartbeatInterval"] = interval
        self.emit('call', {'*': ['slotInstanceNew']},
                  self.deviceId, self.info)
        self.alive = True
        try:
            while True:
                self.heartbeat(interval)
                yield from sleep(interval)
        finally:
            self.emit('call', {'*': ['slotInstanceGone']},
                      self.deviceId, self.info)
            self.alive = False

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
        p['hostname'] = socket.gethostname()
        p['classId'] = self.classId
        self.send(p, args)

    def log(self, message):
        p = openmq.Properties()
        p["target"] = "log"
        m = openmq.BytesMessage()
        m.data = Hash("messages", [message]).encode("Bin")
        m.properties = p
        self.producer.send(m, 1, 4, 100000)

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
                try:
                    slots, params = self.decodeMessage(message)
                except:
                    self.logger.exception("malformated message")
                    continue
                try:
                    slots = [getattr(device, s)
                             for s in slots.get(self.deviceId, [])] + \
                            [getattr(device, s) for s in slots.get("*", [])
                             if hasattr(device, s)]
                except AttributeError:
                    self.logger.exception("slot does not exist")
                    continue
                try:
                    for slot in slots:
                        slot.slot(device, message, params)
                except:
                    self.logger.exception(
                        "internal error while executing slot")
                slot = slots = None
        finally:
            consumer.close()

    @coroutine
    def main(self, device):
        """This is the main loop of a device

        A device is running if this coroutine is still running. If you
        want to stop a device, cancel this coroutine. """
        with ExitStack() as self.exitStack:
            device = weakref.ref(device)
            try:
                yield from self.consume(device())
            finally:
                me = Task.current_task()
                tasks = [t for t in self.tasks if t is not me]
                for t in tasks:
                    t.cancel()
                yield from gather(*tasks, return_exceptions=True)

    def enter_context(self, context):
        return self.exitStack.enter_context(context)

    def updateInstanceInfo(self, info):
        """update the short information about this instance

        the instance info hash contains a very brief summary of the device.
        It is regularly published, and even lives longer than a device,
        as it is published with the message that the device died."""
        self.info.merge(info)
        if self.alive:
            self.emit("call", {"*", ["slotInstanceUpdated"]},
                      self.deviceId, self.info)

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


class NoEventLoop(AbstractEventLoop):
    Queue = queue.Queue
    sync_set = True

    def __init__(self, instance):
        self._instance = instance
        self._cancelled = False
        self.task = None

    def cancel(self):
        self._cancelled = True
        if self.task is not None:
            self._instance._ss.loop.call_soon_threadsafe(self.task.cancel)

    def _sync(self, coro, l1, l2):
        if isinstance(coro, Future):
            self.task = coro
        else:
            self.task = self._instance._ss.loop.create_task(
                coro, instance=self._instance)
        self.task.add_done_callback(lambda _: l2.release())
        l1.release()

    def sync(self, coro, timeout=-1):
        if self._cancelled:
            raise CancelledError
        l1 = threading.Lock()
        l1.acquire()
        l2 = threading.Lock()
        l2.acquire()
        loop = self._instance._ss.loop
        loop.call_soon_threadsafe(self._sync, coro, l1, l2)
        l2.acquire(timeout=timeout)
        l1.acquire()
        try:
            if self.task.done():
                return self.task.result()
            else:
                loop.call_soon_threadsafe(self.task.cancel)
                raise TimeoutError
        finally:
            self.task = None

    def instance(self):
        return self._instance


class EventLoop(SelectorEventLoop):
    Queue = Queue
    sync_set = False

    def __init__(self, topic=None):
        super().__init__()
        if topic is not None:
            self.topic = topic
        elif "KARABO_BROKER_TOPIC" in os.environ:
            self.topic = os.environ["KARABO_BROKER_TOPIC"]
        else:
            self.topic = getpass.getuser()
        self.connection = None
        self.changedFutures = set()  # call if some property changes
        self.set_default_executor(ThreadPoolExecutor(200))
        self.set_exception_handler(EventLoop.exceptionHandler)

    def exceptionHandler(self, context):
        try:
            instance = context["future"].instance()
            instance._onException(None, context["exception"],
                                  context.get("source_traceback"))
        except:
            self.default_exception_handler(context)

    def getBroker(self, deviceId, classId):
        if self.connection is None:
            hosts = os.environ.get("KARABO_BROKER_HOSTS",
                                   "exfl-broker.desy.de:7777").split(',')
            for hp in hosts:
                host, port = hp.split(':')
                p = openmq.Properties()
                p["MQBrokerHostName"] = host.strip()
                p["MQBrokerHostPort"] = int(port)
                p["MQConnectionType"] = "TCP"
                p["MQPingInterval"] = 20
                p["MQSSLIsHostTrusted"] = True
                p["MQAckOnProduce"] = False
                p["MQAckTimeout"] = 0
                try:
                    self.connection = openmq.Connection(p, "guest", "guest")
                    break
                except:
                    self.connection = None
            self.connection.start()

        return Broker(self, deviceId, classId)

    def create_task(self, coro, instance=None):
        """Create a new task, running coroutine *coro*

        As an extension to the standard library method, in Karabo we track
        which device started a task, so that we can cancel them once the
        device goes away.

        *instance* is the device this task should belong to, it defaults
        to the caller's device if existent. Note that a device first
        has to be started with ``startInstance`` before this will work."""
        task = super().create_task(coro)
        try:
            if instance is None:
                instance = get_event_loop().instance()
            instance._ss.tasks.add(task)
            task.add_done_callback(instance._ss.tasks.remove)
            task.instance = weakref.ref(instance, lambda _: task.cancel())
        except (AttributeError, TypeError):
            # create_task has been called from outside a Karabo context
            # this happens in tests and while bootstrapping.
            pass
        return task

    @coroutine
    def start_thread(self, f, *args):
        def inner():
            set_event_loop(loop)
            try:
                return f(*args)
            finally:
                set_event_loop(None)
        loop = NoEventLoop(self.instance())
        return (yield from self.run_in_executor(None, inner))

    def instance(self):
        try:
            return Task.current_task(loop=self).instance()
        except AttributeError:
            return None

    @coroutine
    def waitForChanges(self):
        f = Future(loop=self)
        self.changedFutures.add(f)
        try:
            yield from f
        finally:
            self.changedFutures.remove(f)

    def sync(self, coro, timeout):
        return coro

    def close(self):
        for t in Task.all_tasks(self):
            t.cancel()
        self._ready.extend(self._scheduled)
        self._scheduled.clear()
        while self._ready:
            self._run_once()
        self.connection.close()
        super().close()
