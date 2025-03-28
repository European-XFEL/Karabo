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
import os
import platform
import re
import socket
from asyncio import (
    CancelledError, Future, IncompleteReadError, Lock, Queue, TimeoutError,
    ensure_future, gather, get_event_loop, open_connection, shield,
    start_server, wait_for)
from collections import deque
from contextlib import closing
from ipaddress import ip_address, ip_network
from struct import calcsize, pack, unpack
from weakref import WeakSet

import numpy
from psutil import net_if_addrs

from karabo.native import (
    AccessMode, Assignment, Bool, Configurable, Hash, KaraboError,
    MetricPrefix, Node, Schema, String, UInt16, UInt32, Unit, VectorHash,
    VectorRegexString, VectorString, decodeBinary, encodeBinary, get_timestamp,
    isSet)

from .proxy import ProxyBase, ProxyFactory, ProxyNodeBase, SubProxyBase
from .synchronization import (
    KaraboFuture, background, firstCompleted, sleep, synchronize)

DEFAULT_MAX_QUEUE_LENGTH = 2
IPTOS_LOWDELAY = 0x10
RECONNECT_TIMEOUT = 2
SERVER_WAIT_ONLINE = 5

# Simple pattern to detect IP address
IP_REGEX = (r"(?:[0-9]{1,3}\.){3}[0-9]{1,3}"  # IP
            r"(?:\/[0-9]|[1-2][0-9]|3[0-2])?")  # CIDR
IP_PATTERN = re.compile(IP_REGEX)

# Idle time after which keep-alive mechanism
# start checking the connection
TCP_KEEPIDLE = 30
# Interval between probes keep-alive probes
TCP_KEEPINTVL = 5
# Number of ot acknowledged probes after that the
# connection is considered dead
TCP_KEEPCNT = 5


def set_keep_alive(sock: socket.socket):
    """Set the tcp keep alive on a socket `sock`"""
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, TCP_KEEPIDLE)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, TCP_KEEPINTVL)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, TCP_KEEPCNT)


def get_hostname_from_interface(address_range):
    """returns the ip address of the local interfaces

    in case of bad input, it will throw
    """
    # ip_network will throw if address cannot be found
    network = ip_network(address_range)
    for interface in net_if_addrs().values():
        for nic in interface:
            if nic.family != socket.AF_INET:
                continue
            addr = ip_address(nic.address)
            if addr in network:
                return nic.address
    raise ValueError(
        f"{address_range} does not appear to be an IPv4 or IPv6 network")


def get_network_adapter(name):
    addrs = net_if_addrs()
    for addr in addrs.get(name, []):
        if addr.family is socket.AF_INET:
            return addr.address
    raise ValueError(
        f"{name} cannot be found in network configuration")


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

    def clear(self):
        self.cancel()
        self._queue.clear()


class RingQueue(CancelQueue):
    def _init(self, maxsize=0):
        """Reimplemented function of `Queue`

        The maxsize is forward from the initializer!
        """
        maxlen = int(maxsize) if maxsize >= 0 else None
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
    # Abstract interface configurable
    _allattrs = ["source", "timestamp"]

    source = String(key="source")
    timestamp = Bool(key="timestamp")


class Channel:
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
        # For large chunks avoid StreamReader.readexactly
        ba = bytearray()
        reader = self.reader
        while len(ba) < size:
            b = await reader.read(size - len(ba))
            if not b:
                raise IncompleteReadError(
                    partial=bytes(ba), expected=size)
            ba.extend(b)
        return ba

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
        async with self.drain_lock:
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

    missingConnections = VectorString(
        displayedName="Missing Connections",
        description="Output channels from 'Configured Connections' "
                    "that are not connected",
        defaultValue=[],
        accessMode=AccessMode.READONLY)

    @VectorRegexString(
        displayedName="Configured Connections",
        description="A list of output channels to receive data from, format: "
                    "<instance ID>:<channel name>",
        assignment=Assignment.OPTIONAL, accessMode=AccessMode.INITONLY,
        regex=r"^[a-zA-Z0-9\.\-\:\/\_]+$",
        defaultValue=[])
    async def connectedOutputChannels(self, value):
        # Basic name protection is implemented here, as it can cause hickups!
        outputs = {output.strip() for output in value}
        if not outputs:
            self.connectedOutputChannels = []
            return

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
        self._update_missing_connections(channel, missing=True)
        # Make sure to always set the instance correctly!
        loop = get_event_loop()
        task = loop.create_task(self.start_channel(channel),
                                instance=self.parent.get_root())
        self.connected[channel] = task
        self.connectedOutputChannels = list(self.connected)

    dataDistribution = String(
        displayedName="Data Distribution",
        description="The way data is fetched from the connected output "
                    "channels (shared/copy)",
        options=["shared", "copy"], assignment=Assignment.OPTIONAL,
        defaultValue="copy", accessMode=AccessMode.RECONFIGURABLE)

    @String(
        displayedName="On Slowness",
        description="Policy for what to do if this input is too slow for the "
                    "fed data rate (only used in copy mode, 'queue' means "
                    "'queueDrop')",
        options=["queue", "queueDrop", "drop", "wait"],
        assignment=Assignment.OPTIONAL, defaultValue="drop",
        accessMode=AccessMode.RECONFIGURABLE)
    def onSlowness(self, value):
        self.onSlowness = "queueDrop" if value == "queue" else value

    maxQueueLength = UInt32(
        defaultValue=DEFAULT_MAX_QUEUE_LENGTH,
        minInc=1,
        displayedName="Max. Queue Length Output Channels",
        description="Maximum number of data items to be queued by connected "
                    "Output Channels (only in copy mode and for "
                    "queueDrop policies - Output may force a stricter limit)",
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

    async def connect_handler(self, output):
        # XXX: Please keep this for the time being.
        print(f"NetworkInput connect handler called by {output}!")

    async def close_handler(self, output):
        # XXX: Please keep this for the time being.
        print(f"NetworkInput close handler called by {output}!")

    async def end_of_stream_handler(self, cls):
        # XXX: Please keep this for the time being.
        print(f"EndOfStream handler called by {cls}!")

    async def call_handler(self, func, *args):
        """Call a network input handler under mutex and error protection"""
        async with self.handler_lock:
            try:
                await wait_for(get_event_loop().run_coroutine_or_thread(
                    func, *args), timeout=5)
            except CancelledError:
                # Cancelled Error is handled in the `start_channel` method
                pass
            except TimeoutError:
                self.device_logger.error(
                    f"timeout in handler {func.__name__} in stream")
            except Exception:
                self.device_logger.exception("error in stream")

    @property
    def device_logger(self):
        """Convenience method to retrieve device logger"""
        return self.parent.get_root().logger

    async def start_channel(self, output):
        """Connect to the output channel with Id 'output' """
        try:
            root = self.parent.get_root()
            logger = root.logger
            logger.info(f"Trying to connect to channel {output}")
            instance, name = output.split(":")
            # success, configuration
            ok, info = await root._call_once_alive(
                instance, "slotGetOutputChannelInformation", name, os.getpid())
            if not ok:
                logger.info(
                    f"Connecting to channel that was not there {output}")
                # Start fresh!
                await sleep(RECONNECT_TIMEOUT)
                return (await self.start_channel(output))

            if self.raw:
                cls = None
            else:
                # schema from output channel we are talking with
                schema = info.get("schema", None)
                if schema is None:
                    schema, _ = await root.call(
                        instance, "slotGetSchema", False)
                cls = ProxyFactory.createProxy(
                    Schema(name=name, hash=schema.hash[name]["schema"]))

            reader, writer = await open_connection(
                info["hostname"], int(info["port"]), limit=2 ** 22)

            # Inform about exiting connection
            self._update_missing_connections(output, missing=False)
            sock = writer.get_extra_info("socket")
            assert sock.getsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY)
            # Set low delay according to unix manual IPTOS_LOWDELAY 0x10
            sock.setsockopt(socket.SOL_IP, socket.IP_TOS, IPTOS_LOWDELAY)
            if platform.system() == "Linux":
                set_keep_alive(sock)
            channel = Channel(reader, writer, channelName=output)
            with closing(channel):
                # Tell the world we are connected before we start
                # requesting data
                await shield(self.call_handler(self.connect_handler, output))
                instance_id = f"{root.deviceId}:{self._name}"
                cmd = Hash("reason", "hello",
                           "instanceId", instance_id,
                           "memoryLocation", "remote",
                           "dataDistribution", self.dataDistribution,
                           "onSlowness", self.onSlowness,
                           "maxQueueLength", self.maxQueueLength.value)
                channel.writeHash(cmd)
                while (await self.processChunk(channel, cls, instance_id)):
                    await sleep(self.delayOnInput)
                else:
                    # We still inform when the connection has been closed!
                    self._update_missing_connections(output, missing=True)
                    await shield(self.call_handler(self.close_handler, output))
                    # Start a new roundtrip, but don't create a new task!
                    await sleep(RECONNECT_TIMEOUT)
                    await self.start_channel(output)
        except CancelledError:
            # Note: Happens when we are destroyed or disconnected!
            await shield(self.call_handler(self.close_handler, output))
            self._update_configured_connections(output)
        except Exception:
            logger = self.device_logger
            logger.info(f"Channel `{output}` did not close gracefully.")
            self._update_missing_connections(output, missing=True)
            await shield(self.call_handler(self.close_handler, output))
            # We might get cancelled during sleeping!
            try:
                await sleep(RECONNECT_TIMEOUT)
                await self.start_channel(output)
            except CancelledError:
                self._update_configured_connections(output)
                # Already missing output, no need to update missing

    def _update_configured_connections(self, output):
        """Update and remove channel `output` from connected"""
        # Graceful, due to cancellation it might get out of sync
        self.connected.pop(output, None)
        self.connectedOutputChannels = list(self.connected)

    def _update_missing_connections(self, output, missing):
        """Update the missing connections of the `InputChannel`

        :param missing: Boolean if the connection is missing.
        """
        connections = set(self.missingConnections.value)
        if missing:
            connections.add(output)
        else:
            connections.discard(output)
        self.missingConnections = list(connections)

    async def processChunk(self, channel, cls, output_id=""):
        try:
            header = await channel.readHash()
        except IncompleteReadError as e:
            if e.partial:
                raise
            else:
                root = self.parent.get_root()
                root.logger.info("stream %s finished", channel.channelName)
                return False
        data = await channel.readBytes()
        # `readHash` must happen first, but then we can already
        # request for more updates before processing!
        cmd = Hash("reason", "update",
                   "instanceId", output_id)
        channel.writeHash(cmd)
        if "endOfStream" in header:
            await shield(self.call_handler(
                self.end_of_stream_handler, channel.channelName))
            return True
        pos = 0
        for length, meta_hash in zip(header["byteSizes"],
                                     header["sourceInfo"]):
            chunk = decodeBinary(data[pos:pos + length])
            meta = PipelineMetaData()
            meta._onChanged(meta_hash)
            if self.raw:
                await shield(self.call_handler(self.handler, chunk, meta))
            else:
                proxy = cls()
                proxy._onChanged(chunk)
                await shield(self.call_handler(self.handler, proxy, meta))
            pos += length
        return True


class PipelineContext(NetworkInput):
    """This represents a context specifc input channel connection to a karabo
        device. The context is not connected automatically, but may be
        connected using :meth:`async with` or :meth:`with`::

            channel = PipelineContext("deviceId:output")
            async with channel:
                data, meta = await channel.get_data()

            with channel:
                await channel.get_data()

        It is possible to ask for the connection of the context using::

            async with channel:
                if not channel.is_alive():
                    await channel.wait_connected()
    """

    def __init__(self, output, configuration={}):
        configuration.update(dict(dataDistribution="copy", onSlowness="drop"))
        super().__init__(configuration)
        self.raw = True
        self.parent = self

        self._task = None
        self._output = output
        self._initialized = False

        self._connected = False
        self._queue = CancelQueue()
        self._connect_futures = set()

    # Public interface
    # ----------------------------------------------------------------------

    @synchronize
    async def wait_connected(self):
        """Wait for the pipeline context to be connect

        with PipelineContext(output) as ctx:
            await ctx.wait_connected()

            # do something ...
        """
        if self._connected:
            return True

        future = Future()
        self._connect_futures.add(future)
        return await future

    def is_alive(self):
        """Public method to provide information if we are connected and alive
        """
        return self._connected

    def __enter__(self):
        self._task = background(self._connect())
        return self

    def __exit__(self, exc, value, tb):
        self._disconnect()

    def size(self):
        return self._queue.qsize()

    async def __aenter__(self):
        return self.__enter__()

    async def __aexit__(self, exc, value, tb):
        return self.__exit__(exc, value, tb)

    @synchronize
    async def get_data(self):
        return await self._queue.get()

    # Private interface
    # ----------------------------------------------------------------------

    def get_root(self):
        """Reimplemented function of `Configurable`"""
        return get_event_loop().instance()

    async def connect_handler(self, output):
        """Reimplemented function of `NetworkInput`"""
        self._connected = True
        for future in self._connect_futures:
            future.set_result(True)

        self._connect_futures = set()

    async def close_handler(self, output):
        """Reimplemented function of `NetworkInput`"""
        self._connected = False

    def handler(self, data, meta):
        """Reimplemented function of `NetworkInput`"""
        self._queue.put_nowait((data, meta))

    async def _connect(self):
        output = self._output
        # Similar to the `OutputProxy`, we _run the Configurable and
        # attach the task.
        try:
            if not self._initialized:
                await self._run()
                self._initialized = True
            self.connected[output] = self._task
            await self.start_channel(output)
        finally:
            self._task = None

    def _disconnect(self):
        """Disconnect from the output channel"""
        if self._task is not None and not self._task.done():
            self._task.cancel()
        self._connected = False
        self._queue.clear()


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

    Should it be necessary to act upon `connect`, a handler may be
    installed::

        @input.connect
        def input(self, name):
            # Connect handler has been called by `name`

    :param raw: especially if the sender has not declared the schema of the
        data it sends, one can set `raw` to `True` in order to get the
        uninterpreted hash. The `data` object will be empty if the sender has
        not declared a schema.
    """
    connect_handler = None
    close_handler = None
    end_of_stream_handler = None

    def __init__(self, raw=False, **kwargs):
        super().__init__(NetworkInput, **kwargs)
        self.raw = raw

    def _initialize(self, instance, value):
        """This method is called on initialization

        Called via Configurable checkedInit for every descriptor.

        The `value` still is the bare Hash value, as it came from the network!
        """
        ret = super()._initialize(instance, value)
        channel = instance.__dict__[self.key]
        channel.raw = self.raw
        channel.handler = self.handler.__get__(instance, type(instance))
        channel.parent = instance
        channel._name = self.key
        if self.connect_handler is not None:
            channel.connect_handler = self.connect_handler.__get__(
                instance, type(instance))
        if self.close_handler is not None:
            channel.close_handler = self.close_handler.__get__(
                instance, type(instance))
        if self.end_of_stream_handler is not None:
            channel.end_of_stream_handler = self.end_of_stream_handler.__get__(
                instance, type(instance))
        return ret

    def connect(self, func):
        self.connect_handler = func
        return self

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
        self.longkey = f"{prefix}{key}."
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
        self._connect_handler = None
        self._close_handler = None
        self.networkInput = None

    def setDataHandler(self, handler):
        """Redirect the output of the pipelining proxy before connecting

        NOTE: The handler must take two arguments ``data`` and ``meta``.
        """
        if self.task is not None:
            raise RuntimeError("Setting a data handler must happen before "
                               "connecting to the output channel!")
        self._data_handler = handler

    def setEndOfStreamHandler(self, handler):
        """Redirect the endOfStream of the pipelining proxy before connecting

        NOTE: The handler must take one argument ``channelname``.
        """
        if self.task is not None:
            raise RuntimeError("Setting an endOfStream handler must happen "
                               "before connecting to the output channel!")
        self._eos_handler = handler

    def setConnectHandler(self, handler):
        """Redirect the connect signal of the pipelining proxy

        NOTE: The handler must take one argument ``channelname``.
        """
        if self.task is not None:
            raise RuntimeError("Setting a connected handler must happen "
                               "before connecting to the output channel!")
        self._connect_handler = handler

    def setCloseHandler(self, handler):
        """Redirect the closing signal of the pipelining proxy

        NOTE: The handler must take one argument ``channelname``.
        """
        if self.task is not None:
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
        if self._connect_handler is not None:
            self.networkInput.connect_handler = self._connect_handler
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
        if self.task is not None:
            if isinstance(self.task, KaraboFuture):
                ensure_future(self.task.cancel())
            else:
                self.task.cancel()
            self.task = None


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
                    "distribution is copy: drop, wait, queueDrop")

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

    port = UInt32(
        displayedName="Port",
        description="Port number for TCP connection",
        assignment=Assignment.OPTIONAL,
        defaultValue=0,
        accessMode=AccessMode.INITONLY)

    @String(
        displayedName="No Input (Shared)",
        description="What to do if currently no share-input channel is "
                    "available for writing to ('queue' means 'queueDrop')",
        options=["queue", "drop", "wait", "queueDrop"],
        assignment=Assignment.OPTIONAL, defaultValue="drop",
        accessMode=AccessMode.INITONLY)
    def noInputShared(self, value):
        self.noInputShared = "queueDrop" if value == "queue" else value

    @String(
        displayedName="Hostname",
        description="The hostname which connecting clients will be routed to. "
                    "Network ranges in the CIDR notation are accepted, "
                    "e.g. 0.0.0.0/24",
        assignment=Assignment.OPTIONAL, defaultValue="default",
        accessMode=AccessMode.INITONLY)
    async def hostname(self, value):
        if value == "default":
            hostname = socket.gethostname()
        elif IP_PATTERN.match(value):
            hostname = get_hostname_from_interface(value)
        else:
            hostname = get_network_adapter(value)

        instance = get_event_loop().instance()

        def serve(reader, writer):
            """Create a serve task on the eventloop to track instance

               We need to pass the instance in order to cancel all the tasks
               related to that instance once the instance dies.
            """
            loop = get_event_loop()
            channel_task = loop.create_task(
                self.serve(reader, writer), instance=instance)
            self.active_channels.add(channel_task)
            if not self.alive:
                ts = get_timestamp().toLocal()
                print(f"{ts}: Cancelling channel, going down as instructed.")
                loop.call_soon_threadsafe(channel_task.cancel)

        port = int(self.port) if isSet(self.port) else 0
        self.server = await start_server(serve, host=hostname,
                                         port=port)
        self.hostname = value
        self.address = hostname

    address = String(
        displayedName="Address",
        description="The address resolved from the hostname",
        accessMode=AccessMode.READONLY)

    connections = VectorHash(
        rows=ConnectionTable,
        defaultValue=[],
        displayedName="Connections",
        description="Table of active connections",
        accessMode=AccessMode.READONLY)

    maxQueueLength = UInt32(
        defaultValue=DEFAULT_MAX_QUEUE_LENGTH,
        minInc=2,  # 1 would be equivalent to 'drop' policy
        displayedName="Max. Queue Length",
        description="Maximum queue length accepted from 'copy' mode input "
                    "channels with 'queueDrop' policy",
        accessMode=AccessMode.INITONLY)

    def __init__(self, config):
        super().__init__(config)
        self.copy_queues = []
        self.wait_queues = []
        self.copy_futures = []
        self.active_channels = WeakSet()
        self.has_shared = 0
        self.channelName = ""
        self.shared_queue = CancelQueue(0 if self.noInputShared == "queueDrop"
                                        else 1)
        self.server = None
        # By default, an output is considered alive until closed
        self.alive = True

    def is_serving(self):
        """Return if the output channel can serve data"""
        return self.alive and self.server is not None

    async def wait_server_online(self):
        """Wait until the serving tcp server comes online"""
        total_time = SERVER_WAIT_ONLINE
        interval_time = 0.05  # [s]
        while self.server is None:
            await sleep(interval_time)
            total_time -= interval_time
            if total_time <= 0:
                break
        if self.server is None:
            raise KaraboError("Tcp server of output should be available ...")

    def getInformation(self, channelName):
        self.channelName = channelName
        if self.server is None:
            raise KaraboError(
                f"Trying to get output information for {channelName}, "
                "but output server not available.")
        host, port = self.server.sockets[0].getsockname()
        return Hash("connectionType", "tcp", "hostname", host,
                    "port", numpy.uint32(port))

    async def serve(self, reader, writer):
        channel = Channel(reader, writer, self.channelName)
        distribution = "unknown"
        remote_host = "unknown"
        remote_port = 0
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
            maxQueueLength = min(maxQueueLength,
                                 int(self.maxQueueLength.value))
            if slowness == "queue":  # Pre-Karabo 2.19.0 receiver
                print(f"Queue configuration detected for {channel_name}!")
                slowness = "queueDrop"
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
                if slowness == "queueDrop":
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
        if not self.alive:
            return

        self.alive = False
        await self.wait_server_online()
        # Perform cleanup only after server is online
        for channel_task in list(self.active_channels):
            channel_task.cancel()
            await channel_task

        self.active_channels = WeakSet()
        self.server.close()
        server = self.server
        self.server = None
        await server.wait_closed()


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
            cls.displayType = "OutputSchema"

            class Output(NetworkOutput):
                schema = SchemaNode(cls)

        super().__init__(Output, **kwargs)
