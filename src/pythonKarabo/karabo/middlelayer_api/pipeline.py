from asyncio import (
    coroutine, get_event_loop, IncompleteReadError, Lock, open_connection,
    shield)
from contextlib import closing
import os
from struct import pack, unpack, calcsize

from .enums import Assignment, AccessMode
from .hash import Bool, Hash, VectorString, Schema, String
from .proxy import ProxyBase, ProxyFactory, ProxyNodeBase, SubProxyBase
from .schema import Configurable, Node
from .serializers import decodeBinary, encodeBinary
from .synchronization import background


class PipelineMetaData(ProxyBase):
    source = String(key="source")
    timestamp = Bool(key="timestamp")


class Channel(object):
    """This class is responsible for reading and writing Hashes to TCP"""
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
        pass

    @coroutine
    def call_handler(self, data, meta):
        with (yield from self.handler_lock):
            yield from get_event_loop().run_coroutine_or_thread(
                self.handler, data, meta)

    @coroutine
    def start_channel(self, output):
        try:
            instance, name = output.split(":")
            ok, info = yield from self.parent._call_once_alive(
                instance, "slotGetOutputChannelInformation", name, os.getpid())
            if not ok:
                return

            if self.raw:
                cls = None
            else:
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
