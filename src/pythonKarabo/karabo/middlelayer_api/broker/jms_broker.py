import asyncio
import logging
import socket
import weakref
from asyncio import (
    CancelledError, TimeoutError, ensure_future, gather, get_event_loop,
    shield, sleep, wait_for)
from contextlib import closing
from itertools import count

from karabo.native import Hash, KaraboError, decodeBinary, encodeBinary

from . import openmq
from .base import Broker

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
        self.logger = logging.getLogger(deviceId)
        self.heartbeat_task = None

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
        p["signalInstanceId"] = self.deviceId
        p["__format"] = "Bin"
        m.properties = p
        self.hbproducer.send(m, 1, _MSG_PRIORITY_LOW, _MSG_TIME_TO_LIVE)

    async def notify_network(self, info):
        """notify the network that we are alive

        we send out an instance new and gone, and the heartbeats in between.

        :param info: the info Hash that should be published regularly.
        """
        self.info = info
        self.emit('call', {'*': ['slotInstanceNew']},
                  self.deviceId, self.info)

        async def heartbeat():
            interval = self.info["heartbeatInterval"]
            try:
                while True:
                    await sleep(interval)
                    self.heartbeat(interval)
            except CancelledError:
                pass
            finally:
                self.emit('call', {'*': ['slotInstanceGone']},
                          self.deviceId, self.info)

        self.heartbeat_task = ensure_future(heartbeat())

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

    async def consume_beats(self, server):
        loop = get_event_loop()
        running = True
        server = weakref.ref(server)

        def receiver():
            consumer = openmq.Consumer(
                self.session, self.hbdestination,
                "signalFunction = 'signalHeartbeat'", False)
            with closing(consumer):
                while running:
                    try:
                        message = consumer.receiveMessage(1000)
                    except openmq.Error as e:
                        if e.status == openmq.OPEN_MQ_TIMEOUT:
                            continue
                        elif e.status == openmq.OPEN_MQ_CONCURRENT_ACCESS:
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

                    s = server()
                    if s is None:
                        return
                    hash = decodeBinary(message.data)
                    instance_id, info = hash['a1'], hash['a3']
                    loop.call_soon_threadsafe(
                        loop.create_task, s.updateHeartBeat(instance_id, info))
                    s = None

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

    async def _stop_heartbeat(self):
        if self.heartbeat_task is not None:
            if not self.heartbeat_task.done():
                self.heartbeat_task.cancel()
                await self.heartbeat_task
            self.heartbeat_task = None

    async def stop_tasks(self):
        """Reimplemented method of `Broker`"""
        await self._stop_heartbeat()
        me = asyncio.current_task(loop=None)
        tasks = [t for t in self.tasks if t is not me]
        for t in tasks:
            t.cancel()
        try:
            await wait_for(gather(*tasks, return_exceptions=True),
                           timeout=5)
            return True
        except TimeoutError:
            return False

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
