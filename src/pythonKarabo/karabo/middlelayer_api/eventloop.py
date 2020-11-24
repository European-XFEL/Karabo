from __future__ import absolute_import, unicode_literals

from asyncio import (
    CancelledError, coroutine, ensure_future, Future, get_child_watcher,
    DefaultEventLoopPolicy, gather, get_event_loop, iscoroutinefunction,
    Queue, set_event_loop, shield, sleep, Task, TimeoutError, wait_for)
from concurrent.futures import ThreadPoolExecutor
from contextlib import closing, ExitStack
from functools import wraps
import getpass
import inspect
from itertools import count
import logging
import os
import queue
import socket
import time
import threading
import traceback
import weakref
from abc import ABC, abstractmethod

from karabo.native import KaraboValue, KaraboError, unit_registry as unit
from karabo.native import decodeBinary, encodeBinary, Hash

from .compat import HAVE_ASYNCIO, AbstractEventLoop, SelectorEventLoop
from . import openmq

# See C++ karabo/xms/Signal.hh for reasoning about the two minutes...
_MSG_TIME_TO_LIVE = 120000  # in ms - i.e. 2 minutes
_MSG_PRIORITY_HIGH = 4  # never dropped (except if expired)
_MSG_PRIORITY_LOW = 3  # can be dropped in case of congestion
# Number of threads that can be scheduled in the thread pool executor.
_NUM_THREADS = 200


class Broker(ABC):
    def __init__(self, need_subscribe):
        self.needSubscribe = need_subscribe

    @abstractmethod
    def send(self, p, args):
        pass

    @abstractmethod
    def heartbeat(self, interval):
        pass

    @abstractmethod
    def notify_network(self, info):
        pass

    @abstractmethod
    def call(self, signal, targets, reply, args):
        pass

    @abstractmethod
    def request(self, device, target, *args):
        pass

    @abstractmethod
    def log(self, message):
        pass

    @abstractmethod
    def emit(self, signal, targets, *args):
        pass

    @abstractmethod
    def reply(self, message, reply, error=False):
        pass

    @abstractmethod
    def replyException(self, message, exception):
        pass

    @abstractmethod
    def connect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def disconnect(self, deviceId, signal, slot):
        pass

    @abstractmethod
    def consume(self, device):
        pass

    @abstractmethod
    def handleMessage(self, message, device):
        pass

    def register_slot(self, name, slot):
        pass

    @abstractmethod
    def main(self, device):
        pass

    @abstractmethod
    def stop_tasks(self):
        pass

    @abstractmethod
    def enter_context(self, context):
        pass

    @abstractmethod
    def updateInstanceInfo(self, info):
        pass

    @abstractmethod
    def decodeMessage(self, message):
        pass

    @abstractmethod
    def get_property(self, message, prop):
        return None

    @staticmethod
    def create_connection(hosts, connection):
        # Get scheme (protocol) of first URI...
        scheme, _, _ = hosts[0].split(':')
        if scheme == 'tcp':
            return (JmsBroker.create_connection(hosts, connection),
                    JmsBroker)
        # elif scheme == 'mqtt':
        #    return None, MqttBroker
        else:
            raise RuntimeError(f"Unsupported protocol {scheme}")


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
        self.exitStack = ExitStack()

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

        @coroutine
        def heartbeat():
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
                    yield from sleep(sleepInterval)
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

    @coroutine
    def request(self, device, target, *args):
        reply = "{}-{}".format(self.deviceId, time.monotonic().hex()[4:-4])
        self.call("call", {device: [target]}, reply, args)
        future = Future(loop=self.loop)
        self.repliers[reply] = future
        future.add_done_callback(lambda _: self.repliers.pop(reply))
        return (yield from future)

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

    def connect(self, deviceId, signal, slot):
        self.emit("call", {deviceId: ["slotConnectToSignal"]}, signal,
                  slot.__self__.deviceId, slot.__name__)

    def disconnect(self, deviceId, signal, slot):
        self.emit("call", {deviceId: ["slotDisconnectFromSignal"]}, signal,
                  slot.__self__.deviceId, slot.__name__)

    @coroutine
    def consume(self, device):
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
            yield from shield(task)
        except CancelledError:
            running = False
            yield from task
            raise

    @coroutine
    def handleMessage(self, message, device):
        try:
            slots, params = self.decodeMessage(message)
        except Exception:
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

    @coroutine
    def main(self, device):
        """This is the main loop of a device

        A device is running if this coroutine is still running.
        Use `stop_tasks` to stop this main loop."""
        with self.exitStack:
            # this method should not keep a reference to the device
            # while yielding, otherwise the device cannot be collected
            device = weakref.ref(device)
            yield from self.consume(device())

    @coroutine
    def stop_tasks(self):
        """Stop all currently running task

        This marks the end of life of a device.

        Note that the task this coroutine is called from, as an exception,
        is not cancelled. That's the chicken-egg-problem.
        """
        me = Task.current_task()
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
        yield from wait_for(gather(*tasks, return_exceptions=True),
                            timeout=5)

    def enter_context(self, context):
        return self.exitStack.enter_context(context)

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
                raise RuntimeError(f"All URIs in KARABO_BROKER must"
                                   f" contain the same scheme (protocol)")
            host = host[2:]
            p = openmq.Properties()
            p["MQBrokerHostName"] = host.strip()
            p["MQBrokerHostPort"] = int(port)
            p["MQConnectionType"] = "TCP"
            p["MQPingInterval"] = 20
            p["MQSSLIsHostTrusted"] = True
            p["MQAckOnProduce"] = False
            p["MQAckTimeout"] = 0
            try:
                connection = openmq.Connection(p, "guest", "guest")
                connection.start()
                return connection
            except Exception:
                connection = None
        raise RuntimeError(f"No connection can be established for {hosts}")


def synchronize(coro):
    """Decorate coroutine to play well in threads

    Most Karabo functions are coroutines that must run in the main thread.
    This decorator assures that the decorated coroutine will be called in the
    main thread, and returns the return value to the caller.

    If we are already in the main thread, the coroutine is simply returned.
    """
    coro = coroutine(coro)

    @wraps(coro)
    def wrapper(*args, timeout=-1, wait=True, **kwargs):
        return get_event_loop().sync(coro(*args, **kwargs), timeout, wait)

    if wrapper.__doc__ is not None:
        if not wrapper.__doc__[-1] == "\n":
            wrapper.__doc__ += "\n"
        wrapper.__doc__ += "\nThis is a synchronized coroutine.\n"
    return wrapper


def synchronize_notimeout(coro):
    """same as synchronize, but the timeout is handled by the coroutine"""
    coro = coroutine(coro)

    @wraps(coro)
    def wrapper(*args, wait=True, **kwargs):
        return get_event_loop().sync(coro(*args, **kwargs), -1, wait)

    if wrapper.__doc__ is not None:
        if not wrapper.__doc__[-1] == "\n":
            wrapper.__doc__ += "\n"
        wrapper.__doc__ += "\nThis is a synchronized coroutine.\n"
    return wrapper


class KaraboFuture(object):
    """A handle for a result that will be available in the future

    This will be returned by many Karabo methods, if a callback has been
    set to a function or None. It is possible to add more callbacks, to
    wait for completion, or cancel the operation in question.
    """

    def __init__(self, future):
        self.future = ensure_future(future)

    @synchronize
    def add_done_callback(self, fn):
        """Add another callback to the future"""
        loop = get_event_loop()
        instance = loop.instance()

        def func(future):
            loop.create_task(loop.run_coroutine_or_thread(fn, self), instance)

        self.future.add_done_callback(func)

    @synchronize
    def wait(self):
        """Wait for the result to be available, and return the result"""
        return (yield from self.future)


for f in ["cancel", "cancelled", "done", "result", "exception"]:
    @wraps(getattr(Future, f))
    def method(self, *args, name=f):
        return getattr(self.future, name)(*args)

    setattr(KaraboFuture, f, synchronize(method))


class NoEventLoop(AbstractEventLoop):
    """A fake event loop for a thread

    There is only one Karabo event loop running ever. All other threads
    have this fake event loop running, which is merely a marker that there
    is no event loop. Setting a fake event loop prevents asyncio from
    automatically setting one.
    """
    Queue = queue.Queue
    sync_set = True

    def __init__(self, instance):
        self._instance = instance
        self._cancelled = False
        self.task = None

    def cancel(self):
        """Mark this event loop as cancelled

        This marks the thread of this event loop as cancelled. Subsequent calls
        to Karabo routines will fail with a :exc:`CancelledError`. Currently
        running Karabo routines are cancelled right away.
        """
        self._cancelled = True
        if self.task is not None:
            self._instance._ss.loop.call_soon_threadsafe(self.task.cancel)

    def sync(self, coro, timeout, wait):
        """The main synchronization routine

        This injects the coroutine *coro* into the event loop of the main
        thread, with a *timeout*. If *timeout* is a :class:`KaraboValue` its
        unit is used, otherwise it is in seconds. If *wait* is true, we wait
        for the coroutine to execute, otherwise we return a
        :class:`KaraboFuture`.
        """

        if isinstance(timeout, KaraboValue):
            timeout /= unit.second
        if self._cancelled:
            raise CancelledError
        loop = self._instance._ss.loop
        if wait:
            done = threading.Lock()
            done.acquire()
        hastask = threading.Lock()
        hastask.acquire()

        def inner():
            self.task = loop.create_task(coro, instance=self._instance)
            if wait:
                self.task.add_done_callback(lambda _: done.release())
            hastask.release()

        loop.call_soon_threadsafe(inner)
        if wait:
            done.acquire(timeout=timeout)
            hastask.acquire()
            if self.task.done():
                return self.task.result()
            else:
                loop.call_soon_threadsafe(self.task.cancel)
                raise TimeoutError
        else:
            hastask.acquire()
            future = wait_for(self.task,
                              timeout=timeout if timeout != -1 else None)
            return KaraboFuture(loop.create_task(future,
                                                 instance=self._instance))

    def instance(self):
        return self._instance


class EventLoopPolicy(DefaultEventLoopPolicy):

    def new_loop(self):
        return EventLoop()

    def _loop_factory(self):
        return self.new_loop()


class EventLoop(SelectorEventLoop):
    Queue = Queue
    sync_set = False
    global_loop = None

    def __init__(self, topic=None):
        super().__init__()
        self.connection = None
        if EventLoop.global_loop is not None:
            raise RuntimeError("There can be only one Karabo Eventloop")
        EventLoop.global_loop = self

        if topic is not None:
            self.topic = topic
        elif "KARABO_BROKER_TOPIC" in os.environ:
            self.topic = os.environ["KARABO_BROKER_TOPIC"]
        else:
            self.topic = getpass.getuser()
        if self.topic.endswith("_beats"):
            raise RuntimeError(f"Topic ('{self.topic}') must not end with "
                               f"'_beats'")

        self.changedFutures = set()  # call if some property changes
        self.set_default_executor(ThreadPoolExecutor(_NUM_THREADS))
        self.set_exception_handler(EventLoop.exceptionHandler)

    def exceptionHandler(self, context):
        try:
            instance = context["future"].instance()
            instance._onException(None, context["exception"],
                                  context.get("source_traceback"))
        except Exception:
            self.default_exception_handler(context)

    def getBroker(self, deviceId, classId, broadcast):
        hosts = os.environ.get("KARABO_BROKER",
                               "tcp://exfl-broker.desy.de:7777,"
                               "tcp://localhost:7777").split(',')
        self.connection, Cls = Broker.create_connection(hosts, self.connection)
        return Cls(self, deviceId, classId, broadcast)

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
            task.instance = weakref.ref(instance)
        except (AttributeError, TypeError):
            # create_task has been called from outside a Karabo context
            # this happens in tests and while bootstrapping.
            pass
        return task

    @coroutine
    def run_coroutine_or_thread(self, f, *args, **kwargs):
        """run the function *f* correctly, as a coroutine or thread

        if *f* is a normal function, it is started in a thread and returns
        a future, if it is a coroutine function, it returns the coroutine
        object."""
        if iscoroutinefunction(f):
            return (yield from f(*args, **kwargs))
        else:
            loop = NoEventLoop(self.instance())
            exception = None

            def thread():
                nonlocal exception
                set_event_loop(loop)
                try:
                    ret = f(*args, **kwargs)
                    # The lambda assures we are using the newest future
                    self.call_soon_threadsafe(lambda: future.set_result(ret))
                except Exception as e:
                    exception = e
                    self.call_soon_threadsafe(
                        lambda: future.set_exception(exception))
                finally:
                    if not HAVE_ASYNCIO:
                        # XXX: Loop started in a different thread should be
                        # stopped and closed!
                        loop.stop()
                        loop.close()
                    # Previously we did set loop to `None`. However, since
                    # threads are complaining that no loop is found in
                    # current thread, we might deal with races.
                    # XXX: Thread in thread?
                    # Hence, in case of asyncio we let the `NoEventLoop`
                    # die in the other thread.

            self.run_in_executor(None, thread)
            while True:
                # Why `while` loop???
                # well, it is actually documented some lines down. If this
                # coroutine gets cancelled, we do not want to just give up,
                # but transfer the cancellation to the thread we control.
                # So we catch the exception, send it to the controlled thread
                # instead, and continue. This "and continue" is the while loop
                future = Future(loop=self)
                try:
                    return (yield from future)
                except CancelledError:
                    # Ignore cancelling from the outside, instead cancel the
                    # thread. Forward any resulting exception from the thread
                    # to the caller.
                    if exception is not None:
                        raise
                    else:
                        loop.cancel()

    def start_device(self, device):
        lock = threading.Lock()
        lock.acquire()
        task = None

        @coroutine
        def run_device():
            nonlocal task
            task = Task.current_task()
            try:
                yield from device.startInstance()
            finally:
                lock.release()

        self.call_soon_threadsafe(self.create_task, run_device())
        lock.acquire()
        return task.result()

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
            self.changedFutures.discard(f)

    def something_changed(self):
        for future in self.changedFutures:
            future.set_result(None)
        self.changedFutures = set()

    def sync(self, coro, timeout, wait):
        return coro

    @coroutine
    def cancel_all_tasks(self):
        """Cancel all running tasks except the current executing this"""
        me = Task.current_task()
        tasks = [t for t in Task.all_tasks(self) if t is not me]
        for t in tasks:
            t.cancel()

    def close(self):
        for t in Task.all_tasks(self):
            t.cancel()
        if HAVE_ASYNCIO:
            self._ready.extend(self._scheduled)
            self._scheduled.clear()
            while self._ready:
                self._run_once()
        if self.connection is not None:
            self.connection.close()
        super().close()
        EventLoop.global_loop = None
