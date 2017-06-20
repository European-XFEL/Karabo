from asyncio import (async, coroutine, get_event_loop, Lock, open_connection)
from functools import partial
import os
from struct import pack, unpack, calcsize

from .enums import Assignment, AccessMode
from .hash import Bool, Hash, VectorString, Schema, String
from .schema import Configurable, Node
from .serializers import decodeBinary, encodeBinary

from .proxy import ProxyBase, ProxyFactory


class PipelineMetaData(ProxyBase):
    source = String(key="source")
    timestamp = Bool(key="timestamp")


class Channel(object):
    """This class is responsible for reading and writing Hashes to TCP"""
    sizeCode = "<I"

    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.drain_lock = Lock()

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


class NetworkInput(Configurable):
    """The input channel

    This represents an input channel. It is typically not used directly, instead
    you should just declare a :cls:`InputChannel`.
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
                task = async(self.start_channel(output))
                self.connected[output] = task
                task.add_done_callback(
                    partial(self.connected.pop, output))
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

    @coroutine
    def close_handler(self, cls):
        pass

    @coroutine
    def start_channel(self, output):
        loop = get_event_loop()
        try:
            instance, name = output.split(":")
            ok, info = yield from self.parent._call_once_alive(
                instance, "slotGetOutputChannelInformation", name, os.getpid())
            if not ok:
                return

            if not self.raw:
                schema = info.get("schema")
                if schema is None:
                    schema, _ = yield from self.parent.call(
                        instance, "slotGetSchema", False)

                cls = ProxyFactory.createProxy(
                    Schema(name=name, hash=schema.hash[name]["schema"]))

            channel = Channel(*(yield from open_connection(info["hostname"],
                                                           int(info["port"]))))
            cmd = Hash("reason", "hello", "instanceId", self.parent.deviceId,
                       "memoryLocation", "remote",
                       "dataDistribution", self.dataDistribution,
                       "onSlowness", self.onSlowness)
            channel.writeHash(cmd)
            cmd = Hash("reason", "update", "instanceId", self.parent.deviceId)
            header = yield from channel.readHash()
            while "endOfStream" not in header:
                data = yield from channel.readBytes()
                pos = 0
                for length, meta_hash in zip(header["byteSizes"],
                                             header["sourceInfo"]):
                    chunk = decodeBinary(data[pos:pos + length])
                    meta = PipelineMetaData()
                    meta._onChanged(meta_hash)
                    if self.raw:
                        yield from loop.run_coroutine_or_thread(
                            self.handler, chunk, meta)
                    else:
                        proxy = cls()
                        proxy._onChanged(chunk)
                        yield from loop.run_coroutine_or_thread(
                            self.handler, proxy, meta)
                    pos += length
                channel.writeHash(cmd)
                header = yield from channel.readHash()
        finally:
            self.connected.pop(output)
            self.connectedOutputChannels = list(self.connected)
            yield from loop.run_coroutine_or_thread(self.close_handler, output)


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
