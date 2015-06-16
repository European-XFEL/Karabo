from asyncio import (async, coroutine, Future, Lock, Queue, start_server,
                     open_connection)
import os
import socket
from struct import pack, unpack, calcsize

import numpy

from karabo.enums import Assignment, AccessMode
from karabo.hash import Hash, XMLParser
from karabo.hashtypes import VectorString, String
from karabo.schema import Configurable


class Channel:
    # this is TcpChannel
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
    def readHash(self, encoding):
        r = yield from self.readBytes()
        return Hash.decode(r, encoding)

    def writeHash(self, hash, encoding):
        data = hash.encode(encoding)
        self.writeSize(len(data))
        self.writer.write(data)

    def writeSize(self, size):
        self.writer.write(pack(self.sizeCode, size))

    def drain(self):
        with (yield from self.drain_lock):
            yield from self.writer.drain()


class NetworkOutput(Configurable):
    noInputShared = String(
        displayedName="No Input (Shared)",
        description="What to do if currently no share-input channel is "
                    "available for writing to",
        options=["drop", "queue", "throw", "wait"],
        assignment=Assignment.OPTIONAL, defaultValue="wait",
        accessMode=AccessMode.INITONLY)

    hostname = String(
        displayedName="Hostname",
        description="The hostname which connecting clients will be routed to",
        assignment=Assignment.OPTIONAL, defaultValue="default")

    def __init__(self, config, parent, key):
        super().__init__(config, parent, key)
        self.inputs = Queue()
        self.availableInputs = 0
        self.availability = Future()
        self.queuedChunks = Queue()
        self.chunk = []

    @coroutine
    def start(self):
        if self.hostname == "default":
            self.hostname = socket.gethostname()
        self.server = yield from start_server(self.serve, host=self.hostname,
                                              port=0)

    def getInformation(self):
        host, port = self.server.sockets[0].getsockname()
        return Hash("connectionType", "tcp", "hostname", self.hostname,
                    "port", numpy.uint32(port))

    @coroutine
    def serve(self, reader, writer):
        channel = Channel(reader, writer)

        while True:
            message = yield from channel.readHash("XML")
            reason = message["reason"]

            if reason == "hello":
                channel.instanceId = message["instanceId"]
                channel.memoryLocation = message["memoryLocation"]
                channel.dataDistribution = message["dataDistribution"]
                channel.onSlowness = message["onSlowness"]

            self.availableInputs += 1
            if self.availableInputs == 1:
                self.availability.set_result(None)
                self.availability = Future()
            chunk = yield from self.queuedChunks.get()
            self.availableInputs -= 1
            encoded = [h.encode("Bin") for h in chunk]
            sizes = numpy.array([len(d) for d in encoded], dtype=numpy.uint32)
            h = Hash("nData", numpy.uint32(len(chunk)), "byteSizes", sizes)
            channel.writeHash(h, "XML")
            channel.writeSize(sizes.sum())
            for e in encoded:
                channel.writer.write(e)

    def write(self, data):
        self.chunk.append(data)

    def update_nowait(self):
        if self.availableInputs == 0:
            if self.noInputShared == "drop":
                return
            elif self.noInputShared in ("throw", "wait"):
                raise RuntimeError("data arrives to fast to be processed")
        self.queuedChunks.put_nowait(self.chunk)
        self.chunk = []

    @coroutine
    def update(self):
        if self.noInputShared == "wait" and self.availableInputs == 0:
            yield from self.availability
        self.update_nowait()


class NetworkInput(Configurable):
    @VectorString(
        displayedName="Connected Output Channels",
        description="A list of output channels to receive data from, format: "
                    "<instance ID>:<channel name>",
        assignment=Assignment.OPTIONAL, accessMode=AccessMode.RECONFIGURABLE,
        defaultValue=[])
    def connectedOutputChannels(self, value):
        self.outputs = {(id, name) for id, name in
                        (s.split(":") for s in value)}
        if self.started:
            self.connect()

    dataDistribution = String(
        displayedName="Data Distribution",
        description="The way data is fetched from the connected output "
                    "channels (shared/copy)",
        options=["copy", "shared"], assignment=Assignment.OPTIONAL,
        defaultValue="copy", accessMode=AccessMode.RECONFIGURABLE)

    onSlowness = String(
        displayedName="On Slowness",
        description="Policy for what to do if this input is too slow for the "
                    "fed data rate (only used in copy mode)",
        options=["drop", "throw", "wait", "queue"],
        assignment=Assignment.OPTIONAL, defaultValue="wait",
        accessMode=AccessMode.RECONFIGURABLE)

    def __init__(self, config, parent, key):
        self.started = False
        super().__init__(config, parent, key)
        self.parent = parent

    @coroutine
    def read(self):
        return (yield from self.queue.get())

    def read_nowait(self):
        return self.queue.get_nowait()

    def __len__(self):
        return self.queue.qsize()

    @coroutine
    def start_channel(self, instance, name):
        ok, info = yield from self.parent.call(
            instance, "slotGetOutputChannelInformation", name, os.getpid())
        if not ok:
            return
        channel = Channel(*(yield from open_connection(info["hostname"],
                                                       int(info["port"]))))
        while True:
            h = Hash("reason", "hello", "instanceId", self.parent.deviceId,
                     "memoryLocation", "remote",
                     "dataDistribution", self.dataDistribution,
                     "onSlowness", self.onSlowness)
            channel.writeHash(h, "XML")
            r = yield from channel.readHash("XML")
            if "endOfStream" in r:
                return
            bt = yield from channel.readBytes()
            pos = 0
            for l in r["byteSizes"]:
                yield from self.queue.put(Hash.decode(bt[pos:pos + l], "Bin"))
                pos += l

    @coroutine
    def start(self):
        self.started = True
        self.connected = {}
        self.queue = Queue(maxsize=1)
        self.connect()

    def connect(self):
        close = self.connected.keys() - self.outputs
        for k in close:
            self.connected[k].cancel()
        for instance, name in self.outputs:
            if (instance, name) not in self.connected:
                self.connected[instance, name] = \
                    async(self.start_channel(instance, name))
        self.connectedOutputChannels = ["{}:{}".format(i, n) for i, n in
                                        self.outputs]
