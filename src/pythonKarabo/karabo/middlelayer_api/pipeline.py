from asyncio import (
    CancelledError, Future, gather, get_event_loop,
    IncompleteReadError, Lock, open_connection, Queue, shield,
    start_server)
from collections import deque
from contextlib import closing
import os
from struct import pack, unpack, calcsize
import socket
from weakref import WeakSet

import numpy

from karabo.middlelayer_api.synchronization import sleep
from karabo.native import (
    Assignment, AccessMode, decodeBinary, encodeBinary, Hash, Unit, Schema,
    MetricPrefix)
from karabo.native import (
    Bool, isSet, VectorHash, VectorRegexString, String, UInt16, UInt32)
from karabo.native import Configurable, Node
from karabo.native import get_timestamp

from .proxy import ProxyBase, ProxyFactory, ProxyNodeBase, SubProxyBase
from .synchronization import background, firstCompleted

DEFAULT_MAX_QUEUE_LENGTH = 2


class CancelQueue(Queue):
    """A queue with an added cancel method

    This uses internal variables from the inherited queue, so it may
    not work when upgrading Python in the future. Maybe we can convince
    the Python guys to add this method to the standard library. Worst case
    we have to copy asyncio's Queue.
    """

    def cancel(self):
        """Cancel all getters and putters currently waiting"""
        for fut in self._putters:
            fut.cancel()
        for fut in self._getters:
            fut.cancel()


class RingQueue(CancelQueue):
    def _init(self, maxsize=0):
        """Reimplemented function of `Queue`

        The maxsize is forward from the initializer!
        """
        maxlen = maxsize if maxsize >= 0 else None
        self._queue = deque(maxlen=maxlen)

    def _put(self, item):
        """Reimplemented function of `Queue`"""
        if self.qsize() >= self._maxsize:
            # Declare old task as done!
            self.task_done()
        self._queue.append(item)

    def full(self):
        """Reimplemented function of `Queue`"""
        # XXX: Since we cycle, we should not throw and be full!
        return False


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

    async def readBytes(self):
        buf = (await self.reader.readexactly(calcsize(self.sizeCode)))
        size, = unpack(self.sizeCode, buf)
        return (await self.reader.readexactly(size))

    async def readHash(self):
        r = await self.readBytes()
        return decodeBinary(r)

    def writeHash(self, hsh):
        data = encodeBinary(hsh)
        self.writeSize(len(data))
        self.writer.write(data)

    def writeSize(self, size):
        self.writer.write(pack(self.sizeCode, size))

    async def drain(self):
        with (await self.drain_lock):
            await self.writer.drain()

    def close(self):
        # WHY IS THE READER NOT CLOSEABLE??
        self.writer.close()

    def sendEndOfStream(self):
        h = Hash("endOfStream", True,
                 "byteSizes", numpy.array([], dtype=numpy.uint32))
        # As the messages transporting data, the eos message has a header
        # (i.e. h) and a body. Now this body is empty, but we still have to
        # send its size in bytes, i.e. zero.
        self.writeHash(h)
        self.writeSize(numpy.uint32(0))

    async def nextChunk(self, chunk_future):
        """write a chunk once availabe and wait for next request

        Wait until the chunk_future becomes available, and write it to the
        output. Then wait for the next request.

        This is all in one method such that we can cancel the future for the
        next chunk if the connection is closed. If this happens, an
        IncompleteReadError is raised which has an empty partial data if the
        channel has been closed properly.
        """
        done, pending, error = await firstCompleted(
            chunk=chunk_future, read=self.readHash(), cancel_pending=False)
        if error:
            for future in pending.values():
                future.cancel()
            raise error.popitem()[1]

        if "chunk" in done:
            chunk = done["chunk"]
            encoded = []
            info = []
            n_chunks = 0
            for data, timestamp in chunk:
                if "endOfStream" in data:
                    # XXX: This is safe as an endOfStream is coming
                    # via an extra method! No chunks are available then!
                    self.sendEndOfStream()
                    continue

                n_chunks += 1
                encoded.append(encodeBinary(data))
                # Create the timestamp information for this packet!
                hsh = Hash("source", self.channelName, "timestamp", True)
                hsh["timestamp", ...] = timestamp.toDict()
                info.append(hsh)

            if n_chunks:
                nData = numpy.uint32(n_chunks)
                sizes = numpy.array([len(d) for d in encoded],
                                    dtype=numpy.uint32)
                h = Hash("nData", nData,
                         "byteSizes", sizes,
                         "sourceInfo", info)
                # For future: Replace bytesizes with `bufferSetLayout` and
                # special content to enable copyless deserialization
                self.writeHash(h)
                self.writeSize(sizes.sum())
                self.writer.writelines(encoded)

        if "read" in pending:
            # NOTE: We accept the fact that also ``read`` can be in ``done``!
            # If ``read`` is still ``pending``, we await it here!
            message = await pending["read"]
            assert message["reason"] == "update"
        else:
            message = done["read"]
            assert message["reason"] == "update"
            text = ("{} - Received chunk acknowledgement ``read``. A chunk "
                    "future was retrieved before: {}".format(self.channelName,
                                                             "chunk" in done))
            print(text)
            await sleep(0)


class NetworkInput(Configurable):
    """The input channel

    This represents an input channel. It is typically not used directly,
    instead you should just declare a :cls:`InputChannel`.
    """
    displayType = 'InputChannel'
    classId = 'InputChannel'

    # Internal name to be set by the input channel
    _name = None

    @VectorRegexString(
        displayedName="Connected Output Channels",
        description="A list of output channels to receive data from, format: "
                    "<instance ID>:<channel name>",
        assignment=Assignment.OPTIONAL, accessMode=AccessMode.RECONFIGURABLE,
        regex=r"^[a-zA-Z0-9\.\-\:\/\_]+$",
        defaultValue=[])
    async def connectedOutputChannels(self, value):
        # Basic name protection is implemented here, as it can cause hickups!
        outputs = set(output.strip() for output in value)
        close = self.connected.keys() - outputs
        for k in close:
            self.connected[k].cancel()
        for output in outputs:
            await self.connectChannel(output)

    async def connectChannel(self, channel):
        """Connect to a single Outputchannel
        """
        if channel in self.connected and not self.connected[channel].done():
            return
        # Make sure to always set the instance correctly!
        loop = get_event_loop()
        task = loop.create_task(self.start_channel(channel),
                                instance=self.parent)
        self.connected[channel] = task
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
        options=["queue", "queueDrop", "drop", "wait", "throw"],
        assignment=Assignment.OPTIONAL, defaultValue="drop",
        accessMode=AccessMode.RECONFIGURABLE)

    maxQueueLength = UInt32(
        defaultValue=DEFAULT_MAX_QUEUE_LENGTH,
        minInc=1,
        maxInc=10,
        displayedName="Max. Queue Length Output Channels",
        description="Maximum number of data items to be queued by connected "
                    "Output Channels (only in copy mode and for "
                    "queueDrop policies)",
        accessMode=AccessMode.INITONLY)

    delayOnInput = UInt32(
        defaultValue=0,
        displayedName="Delay on Input channel",
        description="Some delay before informing output channel about "
                    "readiness for next data.",
        unitSymbol=Unit.SECOND,
        metricPrefixSymbol=MetricPrefix.MILLI,
        accessMode=AccessMode.INITONLY)

    def __init__(self, config):
        super().__init__(config)
        self.connected = {}
        self.handler_lock = Lock()

    async def close_handler(self, output):
        # XXX: Please keep this for the time being.
        print("NetworkInput close handler called by {}!".format(output))

    async def end_of_stream_handler(self, cls):
        # XXX: Please keep this for the time being.
        print("EndOfStream handler called by {}!".format(cls))

    async def call_handler(self, data, meta):
        async with self.handler_lock:
            try:
                await get_event_loop().run_coroutine_or_thread(
                    self.handler, data, meta)
            except CancelledError:
                # Cancelled Error is handled in the `start_channel` method
                pass
            except Exception:
                self.parent.logger.exception("error in stream")

    async def start_channel(self, output):
        """Connect to the output channel with Id 'output' """
        try:
            instance, name = output.split(":")
            # success, configuration
            ok, info = await self.parent._call_once_alive(
                instance, "slotGetOutputChannelInformation", name, os.getpid())
            if not ok:
                return

            if self.raw:
                cls = None
            else:
                # schema from output channel we are talking with
                schema = info.get("schema", None)
                if schema is None:
                    schema, _ = await self.parent.call(
                        instance, "slotGetSchema", False)
                cls = ProxyFactory.createProxy(
                    Schema(name=name, hash=schema.hash[name]["schema"]))

            reader, writer = await open_connection(
                info["hostname"], int(info["port"]))
            channel = Channel(reader, writer, channelName=output)
            with closing(channel):
                instance_id = "{}:{}".format(self.parent.deviceId,
                                             self._name)
                cmd = Hash("reason", "hello",
                           "instanceId", instance_id,
                           "memoryLocation", "remote",
                           "dataDistribution", self.dataDistribution,
                           "onSlowness", self.onSlowness,
                           "maxQueueLength", self.maxQueueLength.value)
                channel.writeHash(cmd)
                cmd = Hash("reason", "update",
                           "instanceId", instance_id)
                while (await self.readChunk(channel, cls)):
                    await sleep(self.delayOnInput)
                    channel.writeHash(cmd)
                else:
                    # We still inform when the connection has been closed!
                    await self.safe_close_handler(output)
                    # Start a new roundtrip, but don't create a new task!
                    await sleep(2)
                    await self.start_channel(output)
        except CancelledError:
            # Note: Happens when we are destroyed or disconnected!
            await self.safe_close_handler(output)
            self.connected.pop(output)
            self.connectedOutputChannels = list(self.connected)
        except Exception as e:
            # Provide a print log for developers! Don't use the logger
            # for exceptions. This should not happen!
            print(f"Experienced unexpected exception {e} in input channel "
                  f"of device {self.parent.deviceId} for {output}")
            await self.safe_close_handler(output)
            # We might get cancelled during sleeping!
            try:
                await sleep(2)
                await self.start_channel(output)
            except CancelledError:
                await self.safe_close_handler(output)
                self.connected.pop(output)
                self.connectedOutputChannels = list(self.connected)

    async def safe_close_handler(self, output):
        """The close handler is called under the handler lock"""
        async with self.handler_lock:
            await shield(get_event_loop().run_coroutine_or_thread(
                self.close_handler, output))

    async def readChunk(self, channel, cls):
        try:
            header = await channel.readHash()
        except IncompleteReadError as e:
            if e.partial:
                raise
            else:
                self.parent.logger.info("stream %s finished",
                                        channel.channelName)
                return False
        data = await channel.readBytes()
        if "endOfStream" in header:
            await shield(get_event_loop().run_coroutine_or_thread(
                self.end_of_stream_handler, channel.channelName))
            return True
        pos = 0
        for length, meta_hash in zip(header["byteSizes"],
                                     header["sourceInfo"]):
            chunk = decodeBinary(data[pos:pos + length])
            meta = PipelineMetaData()
            meta._onChanged(meta_hash)
            if self.raw:
                await shield(self.call_handler(chunk, meta))
            else:
                proxy = cls()
                proxy._onChanged(chunk)
                await shield(self.call_handler(proxy, meta))
            pos += length
        return True

    def setChildValue(self, key, value, descriptor):
        """Set the child values on the Configurable

        Overwrite the method of Configurable to prevent sending values.
        """


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

    Should it be necessary to act upon a channel closing, a close handler may
    be installed::

        @input.close
        def input(self, name):
            # close the input channel `name`

    Should it be necessary to act upon endOfStream, a handler may be
    installed::

        @input.endOfStream
        def input(self, name):
            # EndOfStream handler has been called by `name`

    :param raw: especially if the sender has not declared the schema of the
        data it sends, one can set `raw` to `True` in order to get the
        uninterpreted hash. The `data` object will be empty if the sender has
        not declared a schema.
    """
    close_handler = None
    end_of_stream_handler = None

    def __init__(self, raw=False, **kwargs):
        super(InputChannel, self).__init__(NetworkInput, **kwargs)
        self.raw = raw

    def _initialize(self, instance, value):
        """This method is called on initialization

        Called via Configurable checkedInit for every descriptor.

        The `value` still is the bare Hash value, as it came from the network!
        """
        ret = super(InputChannel, self)._initialize(instance, value)
        channel = instance.__dict__[self.key]
        channel.raw = self.raw
        channel.handler = self.handler.__get__(instance, type(instance))
        channel.parent = instance
        channel._name = self.key
        if self.close_handler is not None:
            channel.close_handler = self.close_handler.__get__(
                instance, type(instance))
        if self.end_of_stream_handler is not None:
            channel.end_of_stream_handler = self.end_of_stream_handler.__get__(
                instance, type(instance))
        return ret

    def close(self, func):
        self.close_handler = func
        return self

    def endOfStream(self, func):
        self.end_of_stream_handler = func
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
    not connected automatically, but may be connected using :meth:`connect`::

        device = connectDevice("someDevice")
        # Assuming the output channel has the key `output`
        device.output.connect()

    The default pipeline settings:

        dataDistribution = "copy"
        onSlowness = "drop"

    By default the proxy applies the meta data timestamp to the received data.
    This can be switched to the Hash timestamp by setting the ``meta``
    attribute to ``False``.

        device = connectDevice("someDevice")
        device.output.meta = False
        device.output.connect()
    """
    schema = None

    def __init__(self):
        self.task = None
        self.initialized = False
        # Define if we apply meta data to the pipeline data on the proxy
        self.meta = True
        self._data_handler = None
        self._eos_handler = None
        self._close_handler = None
        self.networkInput = None

    def setDataHandler(self, handler):
        """Redirect the output of the pipelining proxy before connecting

        NOTE: The handler must take two arguments ``data`` and ``meta``.
        """
        if self.initialized:
            raise RuntimeError("Setting a data handler must happen before "
                               "connecting to the output channel!")
        self._data_handler = handler

    def setEndOfStreamHandler(self, handler):
        """Redirect the endOfStream of the pipelining proxy before connecting

        NOTE: The handler must take one argument ``channelname``.
        """
        if self.initialized:
            raise RuntimeError("Setting an endOfStream handler must happen "
                               "before connecting to the output channel!")
        self._eos_handler = handler

    def setCloseHandler(self, handler):
        """Redirect the closing signal of the pipelining proxy

        NOTE: The handler must take one argument ``channelname``.
        """
        if self.initialized:
            raise RuntimeError("Setting a close handler must happen "
                               "before connecting to the output channel!")
        self._close_handler = handler

    async def handler(self, data, meta):
        # Only apply if we have a schema!
        if self.schema is not None:
            if self.meta and meta.timestamp:
                self._parent._onChanged_timestamp_r(
                    data, meta.timestamp.timestamp, self.schema)
            else:
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

        self.networkInput = NetworkInput(dict(
            dataDistribution="copy", onSlowness="drop"))
        self.networkInput.raw = True
        if self._data_handler is not None:
            self.networkInput.handler = self._data_handler
        else:
            self.networkInput.handler = self.handler
        if self._eos_handler is not None:
            self.networkInput.end_of_stream_handler = self._eos_handler
        if self._close_handler is not None:
            self.networkInput.close_handler = self._close_handler
        output = ":".join((self._parent.deviceId, self.longkey))
        # Add our channel to the proxy tracking
        self._parent._remote_output_channel.add(self)
        self.networkInput.parent = self._parent._device
        # Track task correctly according to the Eventloop
        loop = get_event_loop()
        if not loop.sync_set:
            self.task = loop.create_task(self._connect(output),
                                         instance=self._parent._device)
        else:
            self.task = background(self._connect(output))

    async def _connect(self, output):
        try:
            if not self.initialized:
                await self.networkInput._run()
                self.initialized = True
            self.networkInput.connected[output] = self.task
            await self.networkInput.start_channel(output)
        finally:
            self.task = None

    def disconnect(self):
        """Disconnect from the output channel"""
        if self in self._parent._remote_output_channel:
            self._parent._remote_output_channel.remove(self)
        if self.task is not None and not self.task.done():
            self.task.cancel()


class ConnectionTable(Configurable):
    remoteId = String(
        displayedName="Remote ID",
        description="Id of remote input channel")

    dataDistribution = String(
        displayedName="Distribution",
        description="Data distribution of the input channel: shared or copy")

    onSlowness = String(
        displayedName="On slowness",
        description="Data handling policy in case of slowness if data "
                    "distribution is copy: drop, wait, queue, throw")

    memoryLocation = String(
        displayedName="MemoryLocation",
        description="Cache Memory class location: can be remote or local")

    remoteAddress = String(
        displayedName="Remote IP",
        description="Remote TCP address of active connection")

    remotePort = UInt16(
        displayedName="Remote Port",
        description="Remote port of active connection")

    localAddress = String(
        displayedName="Local IP",
        description="Local TCP address of active connection")

    localPort = UInt16(
        displayedName="Local Port",
        description="Local port of active connection")


class NetworkOutput(Configurable):
    displayType = 'OutputChannel'
    classId = 'OutputChannel'
    server = None

    port = UInt32(
        displayedName="Port",
        description="Port number for TCP connection",
        assignment=Assignment.OPTIONAL,
        defaultValue=0,
        accessMode=AccessMode.INITONLY)

    noInputShared = String(
        displayedName="No Input (Shared)",
        description="What to do if currently no share-input channel is "
                    "available for writing to",
        options=["queue", "drop", "wait", "queueDrop"],
        assignment=Assignment.OPTIONAL, defaultValue="drop",
        accessMode=AccessMode.INITONLY)

    @String(
        displayedName="Hostname",
        description="The hostname which connecting clients will be routed to",
        assignment=Assignment.OPTIONAL, defaultValue="default",
        accessMode=AccessMode.INITONLY)
    async def hostname(self, value):
        if value == "default":
            hostname = socket.gethostname()
        else:
            hostname = value

        instance = get_event_loop().instance()

        def serve(reader, writer):
            """Create a serve task on the eventloop to track instance

               We need to pass the instance in order to cancel all the tasks
               related to that instance once the instance dies.
            """
            channel_task = get_event_loop().create_task(
                self.serve(reader, writer), instance)
            self.active_channels.add(channel_task)

        port = int(self.port) if isSet(self.port) else 0
        self.server = await start_server(serve, host=hostname,
                                         port=port)
        self.hostname = value

    connections = VectorHash(
        rows=ConnectionTable,
        defaultValue=[],
        displayedName="Connections",
        description="Table of active connections",
        accessMode=AccessMode.READONLY)

    def __init__(self, config):
        super().__init__(config)
        self.copy_queues = []
        self.wait_queues = []
        self.copy_futures = []
        self.active_channels = WeakSet()
        self.has_shared = 0
        self.shared_queue = CancelQueue(0 if self.noInputShared
                                        in ["queue", "queueDrop"] else 1)

    async def getInformation(self, channelName):
        self.channelName = channelName
        # We are called when we just started, hence we wait for our
        # server to come online!
        while self.server is None:
            await sleep(0.05)
        host, port = self.server.sockets[0].getsockname()
        return Hash("connectionType", "tcp", "hostname", host,
                    "port", numpy.uint32(port))

    async def serve(self, reader, writer):
        channel = Channel(reader, writer, self.channelName)
        distribution = "unknown"
        try:
            message = await channel.readHash()
            assert message["reason"] == "hello"

            # Start parsing the hello message!
            channel_name = message["instanceId"]
            distribution = message["dataDistribution"]
            slowness = message["onSlowness"]
            maxQueueLength = int(message.get("maxQueueLength",
                                             DEFAULT_MAX_QUEUE_LENGTH))
            # Protect against unnecessary high values from outside!
            # The MDL does not have a memory queue ...
            maxQueueLength = min(maxQueueLength, 10)
            if slowness == "throw":
                print(f"Throw configuration detected for {channel_name}!")
                slowness = "drop"
            local_host, local_port = writer.get_extra_info("sockname")
            remote_host, remote_port = writer.get_extra_info("peername")
            # Set the connection table entry
            entry = (channel_name, distribution, slowness, "remote",
                     remote_host, remote_port, local_host, local_port)
            self.connections.extend(entry)

            if distribution == "shared":
                self.has_shared += 1
                while True:
                    await channel.nextChunk(self.shared_queue.get())
            elif slowness == "drop":
                while True:
                    future = Future()
                    self.copy_futures.append(future)
                    await channel.nextChunk(future)
            else:
                if slowness == "queue":
                    queue = CancelQueue()
                elif slowness == "queueDrop":
                    queue = RingQueue(maxQueueLength)
                else:
                    queue = CancelQueue(1)
                if slowness == "wait":
                    queues = self.wait_queues
                else:
                    queues = self.copy_queues
                queues.append(queue)
                try:
                    while True:
                        await channel.nextChunk(queue.get())
                finally:
                    queues.remove(queue)
                    queue.cancel()
        except CancelledError:
            # If we are cancelled, we should close ourselves
            # Note: This happens when the output channel is closed and
            # when the device is destroyed.
            await self.close()
        except IncompleteReadError as e:
            if e.partial:  # if the input got properly closed, partial is empty
                raise
        finally:
            # XXX: Rewrite to channel name!
            if distribution == "shared":
                self.has_shared -= 1
                if self.has_shared == 0:
                    # XXX: Make sure nothing is stuck and cancel all futures
                    # where the consumer does not shutdown gracefully!
                    self.shared_queue.cancel()
            for index, row in enumerate(self.connections.value):
                if (row['remoteAddress'] == remote_host and
                        row['remotePort'] == remote_port):
                    del self.connections[index]
            channel.close()

    def writeChunkNoWait(self, chunk):
        if (self.has_shared > 0 and self.noInputShared != "wait"
                and not self.shared_queue.full()):
            self.shared_queue.put_nowait(chunk)
        for future in self.copy_futures:
            if not future.done():
                future.set_result(chunk)
        self.copy_futures = []
        for queue in self.copy_queues:
            queue.put_nowait(chunk)

    async def writeChunk(self, chunk):
        tasks = [sleep(0)]
        try:
            self.writeChunkNoWait(chunk)
            if self.has_shared > 0 and self.noInputShared == "wait":
                tasks.append(self.shared_queue.put(chunk))
            for queue in self.wait_queues:
                tasks.append(queue.put(chunk))
        finally:
            await gather(*tasks, return_exceptions=True)

    async def writeEndOfStream(self):
        """Send an endOfStream to the clients"""
        timestamp = get_timestamp()
        await self.writeChunk([(Hash("endOfStream", True), timestamp)])

    async def writeData(self, timestamp=None):
        """Send the applied values with given output schema to the clients

        Requires an output channel with a schema
        """
        hsh = self.schema.configurationAsHash()
        if timestamp is None:
            timestamp = get_timestamp()
        await self.writeChunk([(hsh, timestamp)])

    async def writeRawData(self, hsh, timestamp=None):
        """Send raw hash data via the output channel to the clients

        This method can be used if the output channel is used in 'raw' mode,
        e.g. does not have a schema.
        """
        assert isinstance(hsh, Hash)

        if timestamp is None:
            timestamp = get_timestamp()
        await self.writeChunk([(hsh, timestamp)])

    def writeDataNoWait(self, timestamp=None):
        hsh = self.schema.configurationAsHash()
        if timestamp is None:
            timestamp = get_timestamp()
        self.writeChunkNoWait([(hsh, timestamp)])

    def writeRawDataNoWait(self, hsh, timestamp=None):
        assert isinstance(hsh, Hash)

        if timestamp is None:
            timestamp = get_timestamp()
        self.writeChunkNoWait([(hsh, timestamp)])

    def setChildValue(self, key, value, descriptor):
        """Set the child values on the Configurable

        Overwrite the method of Configurable to prevent sending values for
        the pipeline data schema!
        """
        if "." in key:
            return
        super().setChildValue(key, value, descriptor)

    async def close(self):
        """ Cancel all channels and close the server sockets"""
        for channel in self.active_channels:
            if channel and not channel.done():
                channel.cancel()
        self.active_channels = WeakSet()
        # Finally close the async server and the sockets
        if self.server is not None:
            self.server.close()
            await self.server.wait_closed()


class SchemaNode(Node):
    def toDataAndAttrs(self, instance):
        """The Output Channel Schema is not allowed to provide data"""
        return Hash(), {}


class OutputChannel(Node):
    def __init__(self, cls=None, **kwargs):
        if cls is None:
            # Child check for output schema
            Output = NetworkOutput
        else:
            assert issubclass(cls, Configurable)

            class Output(NetworkOutput):
                schema = SchemaNode(cls)

        super(OutputChannel, self).__init__(Output, **kwargs)
