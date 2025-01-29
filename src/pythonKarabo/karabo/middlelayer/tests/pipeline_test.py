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
import socket
from asyncio import (
    Future, IncompleteReadError, StreamReader, StreamWriter, WriteTransport,
    get_running_loop, sleep)
from struct import pack
from zlib import adler32

import pytest
import pytest_asyncio
from psutil import net_if_addrs

from karabo.middlelayer import (
    Channel, Hash, Int32, NetworkInput, NetworkOutput, Proxy, RingQueue,
    background, encodeBinary)
from karabo.middlelayer.pipeline import IP_PATTERN, get_hostname_from_interface
from karabo.middlelayer.testing import run_test


class ChecksumTransport(WriteTransport):
    def __init__(self):
        self.closed = False
        self.checksum = adler32(b"")
        self._extra = {"sockname": ("Local", 8080),
                       "peername": ("Remote", 8080)}

    def write(self, data):
        self.checksum = adler32(data, self.checksum)

    def close(self):
        self.closed = True

    def is_closing(self):
        return self.closed


class FakeTimestamp:
    """as the order of a dictionary is not defined in Python 3.4, we cannot
    use normal timestamps, as their serialization depends on dictionary order.
    This fake timestamp always serializes the same way"""

    def toDict(self):
        return {"a": 3}


class ChannelContext:

    sample_data = [(Hash("a", 5), FakeTimestamp()),
                   (Hash("b", 7), FakeTimestamp())]
    eos_channel = ""

    def __init__(self, channel) -> None:
        self.channel = channel
        self.reader = channel.reader
        self.writer = channel.writer

    def assertChecksum(self, checksum):
        assert self.writer.transport.checksum == checksum

    def feedHash(self, hsh):
        encoded = encodeBinary(hsh)
        self.reader.feed_data(pack("<I", len(encoded)))
        self.reader.feed_data(encoded)

    def feedRequest(self):
        self.feedHash(Hash("reason", "update"))

    async def write_something(self, output, number=10):
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        for i in range(number):
            output.writeChunkNoWait(self.sample_data)
            await self.sleep()
        return task

    def feedTestData(self):
        encoded = encodeBinary(Hash("a", 7))
        meta = Hash("source", "a", "timestamp", False)
        self.feedHash(Hash("byteSizes", [len(encoded)] * 3,
                           "sourceInfo", [meta] * 3))
        self.reader.feed_data(pack("<I", 3 * len(encoded)))
        for i in range(3):
            self.reader.feed_data(encoded)

    def test_writeHash(self):
        h = Hash("b", 7)
        self.channel.writeHash(h)
        self.assertChecksum(124256394)

    async def handler(self, data, meta):
        self.data.append(data)
        self.meta.append(meta)

    async def eos_handler(self, name):
        self.eos_channel = name

    async def sleep(self):
        for _ in range(10):
            await sleep(0)


def test_ip_pattern():
    valid = ["192.168.1.1", "10.0.0.1", "172.16.0.1"]
    for ip_address in valid:
        assert IP_PATTERN.match(ip_address) is not None

    cidr_valid = ["192.168.1.1/24", "10.0.0.1/16", "172.16.0.1/12"]
    for ip_address in cidr_valid:
        assert IP_PATTERN.match(ip_address) is not None

    invalid = ["192.168.1", "192.168.1/2",
               "abc.def.ghi.jkl"]
    for ip_address in invalid:
        assert IP_PATTERN.match(ip_address) is None

    wrong_ips = ["192.168.1.1/33", "10.0.0.1/40", "172.16.0.1/-1",
                 "256.256.256.256", "192.168.1.1/asdf"]
    # These wrong ip's will raise on network resolve
    for ip_address in wrong_ips:
        assert IP_PATTERN.match(ip_address) is not None
        with pytest.raises(ValueError):
            get_hostname_from_interface(ip_address)


@pytest_asyncio.fixture(scope="function", loop_scope="module")
async def channelContext():
    loop = get_running_loop()
    reader = StreamReader(loop=loop)
    writer = StreamWriter(ChecksumTransport(), None, None, loop=loop)
    channel = Channel(reader, writer, "channelname")
    ctx = ChannelContext(channel)
    yield ctx


@run_test
def test_writeSize(channelContext):
    channelContext.channel.writeSize(218)
    channelContext.assertChecksum(57409755)


@run_test
async def test_ring_queue():
    size = 5
    ring = RingQueue(size)
    # Drop 0
    for i in range(size + 1):
        ring.put_nowait(i)

    assert ring.qsize() == size
    items = [ring.get_nowait() for _ in range(size)]
    assert items == [1, 2, 3, 4, 5]

    ring = RingQueue(size)
    for i in range(size + 1):
        ring.put_nowait(i)

    items = []
    for _ in range(size):
        item = await ring.get()
        items.append(item)
    assert items == [1, 2, 3, 4, 5]


@run_test
async def test_readBytes(channelContext):
    channelContext.reader.feed_data(b"\04\00\00\00abcd")
    data = await channelContext.channel.readBytes()
    assert data == b"abcd"


@run_test
async def test_readHash(channelContext):
    channelContext.feedHash(Hash("a", 5))
    data = await channelContext.channel.readHash()
    assert data["a"] == 5


@run_test
async def test_nextChunk(channelContext):
    future = Future()
    task = background(channelContext.channel.nextChunk(future))
    await channelContext.sleep()
    channelContext.assertChecksum(1)
    future.set_result(channelContext.sample_data)
    await channelContext.sleep()
    channelContext.assertChecksum(3020956469)
    assert not task.done()
    channelContext.feedHash(Hash("reason", "update"))
    await task


@run_test
async def test_nextChunk_eof(channelContext):
    future = Future()
    task = background(channelContext.channel.nextChunk(future))
    await channelContext.sleep()
    channelContext.reader.feed_eof()
    try:
        await task
    except IncompleteReadError as error:
        assert not error.partial
    else:
        channelContext.fail()


@run_test
async def test_processChunk(channelContext):
    network = NetworkInput({})
    network.handler = channelContext.handler
    network.raw = False
    await network._run()

    class Test(Proxy):
        a = Int32()

    task = background(network.processChunk(channelContext.channel, Test))
    channelContext.feedTestData()
    channelContext.data = []
    channelContext.meta = []
    assert (await task)
    assert len(channelContext.data) == 3
    assert channelContext.data[1].a == 7
    assert len(channelContext.meta) == 3
    assert channelContext.meta[2].source == "a"
    assert not channelContext.meta[0].timestamp


@run_test
async def test_processChunk_raw(channelContext):
    network = NetworkInput({})
    network.raw = True
    network.handler = channelContext.handler
    await network._run()
    task = background(network.processChunk(channelContext.channel, None))
    channelContext.feedTestData()
    channelContext.data = []
    channelContext.meta = []
    assert (await task)
    assert len(channelContext.data) == 3
    assert channelContext.data[1]["a"] == 7
    assert len(channelContext.meta) == 3
    assert channelContext.meta[2].source == "a"
    assert not channelContext.meta[0].timestamp


@run_test
async def test_processChunk_eos(channelContext):
    network = NetworkInput({})
    network.raw = True
    network.handler = channelContext.handler
    network.end_of_stream_handler = channelContext.eos_handler
    await network._run()
    assert channelContext.eos_channel == ""
    task = background(network.processChunk(channelContext.channel, None))
    channelContext.feedHash(Hash("endOfStream", True))
    channelContext.reader.feed_data(b"\0\0\0\0")
    channelContext.data = []
    channelContext.meta = []
    assert (await task)
    assert channelContext.data == []
    assert channelContext.eos_channel == "channelname"
    assert len(channelContext.meta) == 0


@run_test
async def test_processChunk_eof(channelContext, mocker):
    network = NetworkInput({})
    network.parent = mocker.Mock()
    await network._run()
    channelContext.reader.feed_eof()
    assert not (await network.processChunk(channelContext.channel, None))


@run_test
async def test_shared_drop(channelContext):
    output = NetworkOutput({"noInputShared": "drop"})
    await output._run()
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "shared",
             "onSlowness", "drop", "instanceId", "Test"))
    task = await channelContext.write_something(output)
    await channelContext.sleep()
    channelContext.assertChecksum(3020956469)
    channelContext.feedRequest()
    await channelContext.sleep()
    channelContext.feedRequest()
    for _ in range(100):
        channelContext.assertChecksum(452019817)
        await channelContext.sleep()
    output.writeChunkNoWait(channelContext.sample_data)
    await channelContext.sleep()
    channelContext.assertChecksum(881158557)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
async def test_shared_queue_drop(channelContext):
    output = NetworkOutput({"noInputShared": "queueDrop"})
    await output._run()
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "shared",
             "onSlowness", "queueDrop", "instanceId", "Test"))
    task = await channelContext.write_something(output, 200)
    await channelContext.sleep()
    channelContext.assertChecksum(3020956469)
    channelContext.feedRequest()
    await channelContext.sleep()
    channelContext.feedRequest()
    await channelContext.sleep()
    for _ in range(100):
        channelContext.assertChecksum(881158557)
        await channelContext.sleep()
    output.writeChunkNoWait(channelContext.sample_data)
    await channelContext.sleep()
    channelContext.assertChecksum(881158557)
    for i in range(200):
        output.writeChunkNoWait(channelContext.sample_data)
    await channelContext.sleep()
    channelContext.assertChecksum(881158557)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
def test_shared_throw(channelContext):
    with pytest.raises(ValueError):
        output = NetworkOutput({"noInputShared": "throw"})  # noqa


@run_test
async def test_shared_wait(channelContext):
    output = NetworkOutput({"noInputShared": "wait"})
    output.channelName = "channelname"
    await output._run()
    task = background(
        output.serve(
            channelContext.reader,
            channelContext.writer))
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "shared",
             "onSlowness", "wait", "instanceId", "Test"))
    await channelContext.sleep()
    await output.writeChunk([(Hash("a", 5), FakeTimestamp())])
    await channelContext.sleep()
    channelContext.feedRequest()
    await channelContext.sleep()
    await output.writeChunk(channelContext.sample_data)
    await output.writeChunk(channelContext.sample_data)
    waiter = background(output.writeChunk(channelContext.sample_data))
    await channelContext.sleep()
    assert not waiter.done()
    channelContext.feedRequest()
    await channelContext.sleep()
    assert waiter.done()
    channelContext.assertChecksum(2194103602)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
async def test_copy_drop(channelContext):
    output = NetworkOutput({"noInputShared": "drop"})
    await output._run()
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "copy",
             "onSlowness", "drop", "instanceId", "Test"))
    task = await channelContext.write_something(output)
    await channelContext.sleep()
    channelContext.assertChecksum(3020956469)
    channelContext.feedRequest()
    await channelContext.sleep()
    for _ in range(100):
        channelContext.assertChecksum(3020956469)
        await channelContext.sleep()
    output.writeChunkNoWait(channelContext.sample_data)
    channelContext.feedRequest()
    await channelContext.sleep()
    channelContext.assertChecksum(452019817)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
async def test_copy_queue_drop(channelContext):
    output = NetworkOutput({"noInputShared": "drop"})
    await output._run()
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "copy",
             "onSlowness", "queueDrop", "instanceId", "Test"))
    task = await channelContext.write_something(output, 200)
    await channelContext.sleep()
    channelContext.assertChecksum(3020956469)
    # Write like crazy, we do not have a request and roll!
    output.writeChunkNoWait(channelContext.sample_data)
    output.writeChunkNoWait(channelContext.sample_data)
    output.writeChunkNoWait(channelContext.sample_data)
    channelContext.assertChecksum(3020956469)
    channelContext.feedRequest()
    await channelContext.sleep()
    for _ in range(100):
        channelContext.assertChecksum(452019817)
        await channelContext.sleep()
    output.writeChunkNoWait(channelContext.sample_data)
    channelContext.feedRequest()
    await channelContext.sleep()
    channelContext.assertChecksum(881158557)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
async def test_copy_queue_drop_large_max_queue(channelContext):
    """Test the queue drop with a high max queue length"""
    output = NetworkOutput({"noInputShared": "queueDrop"})
    await output._run()
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "copy",
             "onSlowness", "queueDrop", "instanceId", "Test",
             "maxQueueLength", 200))
    task = await channelContext.write_something(output, 200)
    await channelContext.sleep()
    channelContext.assertChecksum(3020956469)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
async def test_copy_wait(channelContext):
    output = NetworkOutput({"noInputShared": "drop"})
    await output._run()
    output.channelName = "channelname"
    task = background(
        output.serve(
            channelContext.reader,
            channelContext.writer))
    channelContext.feedHash(
        Hash("reason", "hello", "dataDistribution", "copy",
             "onSlowness", "wait", "instanceId", "Test"))
    await output.writeChunk(channelContext.sample_data)
    await channelContext.sleep()
    await output.writeChunk(channelContext.sample_data)
    channelContext.feedRequest()
    await channelContext.sleep()
    await output.writeChunk(channelContext.sample_data)
    await output.writeChunk(channelContext.sample_data)
    waiter = background(output.writeChunk(channelContext.sample_data))
    await channelContext.sleep()
    assert not waiter.done()
    channelContext.feedRequest()
    await channelContext.sleep()
    assert waiter.done()
    channelContext.assertChecksum(881158557)
    channelContext.reader.feed_eof()
    await task
    assert channelContext.writer.transport.closed


@run_test
async def test_get_information(channelContext):
    net_segment = None
    ip_address = None
    for interface in net_if_addrs().values():
        for nic in interface:
            if nic.family != socket.AF_INET:
                continue
            ip_address = nic.address
            return ip_address

    assert net_segment is not None
    assert ip_address is not None
    output = NetworkOutput({"hostname": net_segment})
    await output._run()
    response = output.getInformation("channelName")
    assert "connectionType" in response
    assert "port" in response
    assert response["hostname"] == ip_address
    assert output.address == ip_address
