from asyncio import (
    coroutine, Future, gather, get_event_loop, IncompleteReadError, Lock,
    open_connection, Queue, QueueFull, shield, sleep, start_server)
from contextlib import closing
import os
from struct import pack, unpack, calcsize
import socket

import numpy

from .enums import Assignment, AccessMode
from .hash import Bool, Hash, VectorString, Schema, String
from .proxy import ProxyBase, ProxyFactory, ProxyNodeBase, SubProxyBase
from .schema import Configurable, Node
from .serializers import decodeBinary, encodeBinary
from .synchronization import background, firstCompleted
from .timestamp import Timestamp


class PipelineMetaData(ProxyBase):
    source = String(key="source")
    timestamp = Bool(key="timestamp")


class Channel(object):
    """This class is responsible for reading and writing Hashes to TCP

    It is the low-level implementation of the pipeline protocol."""
    sizeCode = "<I"

    def __init__(self, reader, writer, channelName=None):
        self.reader = reader
        self.writer = writer
        self.drain_lock = Lock()
        self.channelName = channelName

    @coroutine
    def readBytes(self):
        buf = (yield from self.reader.readexactly(calcsize(self.sizeCode)))
        size, = unpack(self.sizeCode, buf)
        return (yield from self.reader.readexactly(size))

    @coroutine
    def readHash(self):
        r = yield from self.readBytes()
        return decodeBinary(r)

    def writeHash(self, hsh):
        data = encodeBinary(hsh)
        self.writeSize(len(data))
        self.writer.write(data)

    def writeSize(self, size):
        self.writer.write(pack(self.sizeCode, size))

    @coroutine
    def drain(self):
        with (yield from self.drain_lock):
            yield from self.writer.drain()

    def close(self):
        # WHY IS THE READER NOT CLOSEABLE??
        self.writer.close()

    @coroutine
    def nextChunk(self, chunk_future):
        """write a chunk once availabe and wait for next request

        Wait until the chunk_future becomes available, and write it to the
        output. Then wait for the next request.

        This is all in one method such that we can cancel the future for the
        next chunk if the connection is closed. If this happens, an
        IncompleteReadError is raised which has an empty partial data if the
        channel has been closed properly.
        """
        done, pending, error = yield from firstCompleted(
            chunk=chunk_future, read=self.readHash(), cancel_pending=False)
        if error:
            for future in pending.values():
                future.cancel()
            raise error.popitem()[1]
        chunk = done["chunk"]
        encoded = [encodeBinary(h[0]) for h in chunk]
        sizes = numpy.array([len(d) for d in encoded], dtype=numpy.uint32)
        info = []
        for _, timestamp in chunk:
            hsh = Hash("source", self.channelName, "timestamp", True)
            hsh["timestamp", ...] = timestamp.toDict()
            info.append(hsh)
        h = Hash("nData", numpy.uint32(len(chunk)), "byteSizes", sizes,
                 "sourceInfo", info)
        self.writeHash(h)
        self.writeSize(sizes.sum())
        for e in encoded:
            self.writer.write(e)
        message = yield from pending["read"]
        assert message["reason"] == "update"


class NetworkInput(Configurable):
    """The input channel

    This represents an input channel. It is typically not used directly,
    instead you should just declare a :cls:`InputChannel`.
    """

    @VectorString(
        displayedName="Connected Output Channels",
        description="A list of output channels to receive data from, format: "
                    "<instance ID>:<channel name>",
        assignment=Assignment.OPTIONAL, accessMode=AccessMode.RECONFIGURABLE,
        defaultValue=[])
    @coroutine
    def connectedOutputChannels(self, value):
        outputs = set(value)
        close = self.connected.keys() - outputs
        for k in close:
            self.connected[k].cancel()
        for output in outputs:
            if output not in self.connected:
                task = background(self.start_channel(output))
                self.connected[output] = task
        self.connectedOutputChannels = list(self.connected)

    dataDistribution = String(
        displayedName="Data Distribution",
        description="The way data is fetched from the connected output "
                    "channels (shared/copy)",
        options=["shared", "copy"], assignment=Assignment.OPTIONAL,
        defaultValue="copy", accessMode=AccessMode.RECONFIGURABLE)

    onSlowness = String(
        displayedName="On Slowness",
        description="Policy for what to do if this input is too slow for the "
                    "fed data rate (only used in copy mode)",
        options=["queue", "drop", "wait", "throw"],
        assignment=Assignment.OPTIONAL, defaultValue="wait",
        accessMode=AccessMode.RECONFIGURABLE)

    def __init__(self, config):
        super().__init__(config)
        self.connected = {}
        self.handler_lock = Lock()

    @coroutine
    def close_handler(self, cls):
        # XXX: Please keep this for the time being.
        print("NetworkInput close handler called by {}!".format(cls))

    @coroutine
    def call_handler(self, data, meta):
        with (yield from self.handler_lock):
            yield from get_event_loop().run_coroutine_or_thread(
                self.handler, data, meta)

    @coroutine
    def start_channel(self, output):
        try:
            instance, name = output.split(":")
            # success, configuration
            ok, info = yield from self.parent._call_once_alive(
                instance, "slotGetOutputChannelInformation", name, os.getpid())
            if not ok:
                return

            if self.raw:
                cls = None
            else:
                # schema from output channel
                schema = info.get("schema")
                if schema is None:
                    schema, _ = yield from self.parent.call(
                        instance, "slotGetSchema", False)

                cls = ProxyFactory.createProxy(
                    Schema(name=name, hash=schema.hash[name]["schema"]))

            reader, writer = yield from open_connection(
                info["hostname"], int(info["port"]))
            channel = Channel(reader, writer, channelName=output)
            with closing(channel):
                cmd = Hash("reason", "hello",
                           "instanceId", self.parent.deviceId,
                           "memoryLocation", "remote",
                           "dataDistribution", self.dataDistribution,
                           "onSlowness", self.onSlowness)
                channel.writeHash(cmd)
                cmd = Hash("reason", "update",
                           "instanceId", self.parent.deviceId)
                while (yield from self.readChunk(channel, cls)):
                    channel.writeHash(cmd)
        finally:
            self.connected.pop(output)
            self.connectedOutputChannels = list(self.connected)
            with (yield from self.handler_lock):
                yield from shield(get_event_loop().run_coroutine_or_thread(
                                  self.close_handler, output))

    @coroutine
    def readChunk(self, channel, cls):
        try:
            header = yield from channel.readHash()
        except IncompleteReadError as e:
            if e.partial:
                raise
            else:
                self.parent.logger.info("stream %s finished",
                                        channel.channelName)
                return False
        data = yield from channel.readBytes()
        if "endOfStream" in header:
            meta = PipelineMetaData()
            meta._onChanged(Hash("source", channel.channelName))
            yield from shield(self.call_handler(None, meta))
            return True
        pos = 0
        for length, meta_hash in zip(header["byteSizes"],
                                     header["sourceInfo"]):
            chunk = decodeBinary(data[pos:pos + length])
            meta = PipelineMetaData()
            meta._onChanged(meta_hash)
            if self.raw:
                yield from shield(self.call_handler(chunk, meta))
            else:
                proxy = cls()
                proxy._onChanged(chunk)
                yield from shield(self.call_handler(proxy, meta))
            pos += length
        return True


class InputChannel(Node):
    """Declare an input channel in a device

    This descriptor declares an input channel of a devices. It is most
    conveniently used as a decorator to the handler for the incoming data::

       @InputChannel()
       def input(self, data, meta):
           # do something useful with data

    The handler is passed the sent data as well as a meta object which has
    two attributes: ``meta.source`` is the name of the connected output channel
    (device id and channel name join with a colon), while ``meta.timestamp`` is
    a boolean which is always `True`, but has the timestamp of the arriving
    data.

    Should it be necessary to act upon a channel closing, a close hander may be
    installed::

        @input.close
        def input(self, name):
            # close the input channel `name`

    :param raw: especially if the sender has not declared the schema of the
        data it sends, one can set `raw` to `True` in order to get the
        uninterpreted hash. The `data` object will be empty if the sender has
        not declared a schema.
    """
    close_handler = None

    def __init__(self, raw=False, **kwargs):
        super(InputChannel, self).__init__(NetworkInput, **kwargs)
        self.raw = raw

    def _initialize(self, instance, value):
        ret = super(InputChannel, self)._initialize(instance, value)
        channel = instance.__dict__[self.key]
        channel.raw = self.raw
        channel.handler = self.handler.__get__(instance, type(instance))
        channel.parent = instance
        if self.close_handler is not None:
            channel.close_handler = self.close_handler.__get__(
                instance, type(instance))
        return ret

    def close(self, func):
        self.close_handler = func
        return self

    def __call__(self, handler):
        if self.description is None:
            self.description = handler.__doc__
        self.handler = handler
        return self


class OutputProxyNode(ProxyNodeBase):
    """Descriptor representing a Proxy for an output channel"""
    def __init__(self, key, node, prefix, **kwargs):
        self.longkey = "{}{}.".format(prefix, key)
        sub = ProxyFactory.createNamespace(node, self.longkey)
        cls = type(key, (OutputProxy,), sub)
        super().__init__(key=key, cls=cls, **kwargs)

    def __get__(self, instance, owner):
        ret = super().__get__(instance, owner)
        ret.longkey = self.longkey
        return ret


ProxyFactory.node_factories["OutputChannel"] = OutputProxyNode


class OutputProxy(SubProxyBase):
    """A Proxy for an output channel

    This represents an output channel on a device. Output channels are
    not connected automatically, but may be connected using :meth:`connect`.
    """
    def __init__(self):
        self.networkInput = NetworkInput(dict(
            dataDistribution="copy", onSlowness="drop"))
        self.networkInput.raw = True
        self.networkInput.handler = self.handler
        self.task = None
        self.initialized = False

    @coroutine
    def handler(self, data, meta):
        self._parent._onChanged_r(data, self.schema)

    def connect(self):
        """Connect to the output channel

        The connection is always a copy/drop connection, meaning only samples
        of the data reach this proxy, without influencing other consumer.
        For a fully featured input write a device with a
        :class:`InputChannel`.
        """
        if self.task is not None:
            return
        output = ":".join((self._parent.deviceId, self.longkey))
        self.networkInput.parent = self._parent._device
        self.task = background(self._connect(output))

    @coroutine
    def _connect(self, output):
        try:
            if not self.initialized:
                yield from self.networkInput._run()
                self.initialized = True
            self.networkInput.connected[output] = self.task
            yield from self.networkInput.start_channel(output)
        finally:
            self.task = None

    def disconnect(self):
        """Disconnect from the output channel"""
        if self.task is not None:
            self.task.cancel()


class NetworkOutput(Configurable):
    displayType = 'OutputChannel'

    noInputShared = String(
        displayedName="No Input (Shared)",
        description="What to do if currently no share-input channel is "
                    "available for writing to",
        options=["queue", "drop", "wait", "throw"],
        assignment=Assignment.OPTIONAL, defaultValue="wait",
        accessMode=AccessMode.INITONLY)

    @String(
        displayedName="Hostname",
        description="The hostname which connecting clients will be routed to",
        assignment=Assignment.OPTIONAL, defaultValue="default",
        accessMode=AccessMode.INITONLY)
    def hostname(self, value):
        if value == "default":
            self.hostname = socket.gethostname()
        else:
            self.hostname = value
        self.server = yield from start_server(self.serve, host=self.hostname,
                                              port=0)

    def __init__(self, config):
        super().__init__(config)
        self.copy_queues = []
        self.wait_queues = []
        self.copy_futures = []
        self.shared_queue = Queue(0 if self.noInputShared == "queue" else 1)

    def getInformation(self, channelName):
        self.channelName = channelName
        host, port = self.server.sockets[0].getsockname()
        return Hash("connectionType", "tcp", "hostname", self.hostname,
                    "port", numpy.uint32(port))

    @coroutine
    def serve(self, reader, writer):
        channel = Channel(reader, writer, self.channelName)

        try:
            message = yield from channel.readHash()
            assert message["reason"] == "hello"

            assert message["dataDistribution"] in {"shared", "copy"}
            if message["dataDistribution"] == "shared":
                while True:
                    yield from channel.nextChunk(self.shared_queue.get())
            elif message["onSlowness"] == "drop":
                while True:
                    future = Future()
                    self.copy_futures.append(future)
                    yield from channel.nextChunk(future)
            else:
                if message["onSlowness"] == "queue":
                    queue = Queue()
                else:
                    queue = Queue(1)
                if message["onSlowness"] == "wait":
                    queues = self.wait_queues
                else:
                    queues = self.copy_queues
                queues.append(queue)
                try:
                    while True:
                        if queue.full() and message["onSlowness"] == "throw":
                            return
                        yield from channel.nextChunk(queue.get())
                finally:
                    queues.remove(queue)
        except IncompleteReadError as e:
            if e.partial:  # if the input got properly closed, partial is empty
                raise
        finally:
            channel.close()

    def writeChunkNoWait(self, chunk):
        if self.noInputShared != "wait" and not self.shared_queue.full():
            self.shared_queue.put_nowait(chunk)
        elif self.noInputShared == "throw":
            raise QueueFull()
        for future in self.copy_futures:
            if not future.done():
                future.set_result(chunk)
        self.copy_futures = []
        for queue in self.copy_queues:
            queue.put_nowait(chunk)

    @coroutine
    def writeChunk(self, chunk):
        tasks = [sleep(0)]
        try:
            self.writeChunkNoWait(chunk)
            if self.noInputShared == "wait":
                tasks.append(self.shared_queue.put(chunk))
            for queue in self.wait_queues:
                tasks.append(queue.put(chunk))
        finally:
            yield from gather(*tasks)

    @coroutine
    def writeData(self, timestamp=None):
        """Send the applied values with given output schema to the clients

        Requires an output channel with a schema
        """
        hsh = self.schema.configurationAsHash()
        if timestamp is None:
            timestamp = Timestamp()
        yield from self.writeChunk([(hsh, timestamp)])

    @coroutine
    def writeRawData(self, hsh, timestamp=None):
        """Send raw hash data via the output channel to the clients

        This method can be used if the output channel is used in 'raw' mode,
        e.g. does not have a schema.
        """
        assert isinstance(hsh, Hash) and self.schema is None

        if timestamp is None:
            timestamp = Timestamp()
        yield from self.writeChunk([(hsh, timestamp)])

    def writeDataNoWait(self, timestamp=None):
        hsh = self.schema.configurationAsHash()
        if timestamp is None:
            timestamp = Timestamp()
        self.writeChunkNoWait([(hsh, timestamp)])

    def writeRawDataNoWait(self, hsh, timestamp=None):
        assert isinstance(hsh, Hash) and self.schema is None

        if timestamp is None:
            timestamp = Timestamp()
        self.writeChunkNoWait([(hsh, timestamp)])

    def setChildValue(self, key, value, descriptor):
        """Set the child values on the Configurable

        Overwrite the method of Configurable to prevent sending values.
        """


class OutputChannel(Node):
    def __init__(self, cls=None, **kwargs):
        if cls is None:
            # Child check for output schema
            Output = NetworkOutput
        else:
            assert issubclass(cls, Configurable)

            class Output(NetworkOutput):
                schema = Node(cls)

        super(OutputChannel, self).__init__(Output, **kwargs)
