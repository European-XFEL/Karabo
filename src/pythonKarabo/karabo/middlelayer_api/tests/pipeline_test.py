from asyncio import (
    coroutine, Future, IncompleteReadError, QueueFull, sleep, StreamReader,
    StreamWriter, WriteTransport)
from unittest import main
from unittest.mock import Mock
from struct import pack
from zlib import adler32

from karabo.middlelayer import (
    background, encodeBinary, Hash, Int32, NetworkInput, NetworkOutput, Proxy)
from karabo.middlelayer_api.pipeline import Channel, RingQueue

from .eventloop import DeviceTest, async_tst


class ChecksumTransport(WriteTransport):
    def __init__(self):
        self.checksum = adler32(b"")
        self._extra = {'sockname': ("Local", 8080),
                       'peername': ("Remote", 8080)}

    def write(self, data):
        self.checksum = adler32(data, self.checksum)

    def close(self):
        self.closed = True


class FakeTimestamp:
    """as the order of a dictionary is not defined in Python 3.4, we cannot
    use normal timestamps, as their serialization depends on dictionary order.
    This fake timestamp always serializes the same way"""
    def toDict(self):
        return {"a": 3}


class TestChannel(DeviceTest):
    sample_data = [(Hash("a", 5), FakeTimestamp()),
                   (Hash("b", 7), FakeTimestamp())]
    eos_channel = ""

    def setUp(self):
        self.reader = StreamReader()
        self.writer = StreamWriter(ChecksumTransport(), None, None, None)
        self.channel = Channel(self.reader, self.writer, "channelname")

    def assertChecksum(self, checksum):
        self.assertEqual(self.writer.transport.checksum, checksum)

    def feedHash(self, hsh):
        encoded = encodeBinary(hsh)
        self.reader.feed_data(pack("<I", len(encoded)))
        self.reader.feed_data(encoded)

    def feedRequest(self):
        self.feedHash(Hash("reason", "update"))

    @async_tst
    async def test_ring_queue(self):
        size = 5
        ring = RingQueue(size)
        # Drop 0
        for i in range(size + 1):
            ring.put_nowait(i)

        self.assertEqual(ring.qsize(), size)
        items = [ring.get_nowait() for _ in range(size)]
        self.assertEqual(items, [1, 2, 3, 4, 5])

        ring = RingQueue(size)
        for i in range(size + 1):
            ring.put_nowait(i)

        items = []
        for _ in range(size):
            item = await ring.get()
            items.append(item)
        self.assertEqual(items, [1, 2, 3, 4, 5])

    @coroutine
    def sleep(self):
        for _ in range(10):
            yield from sleep(0)

    @async_tst
    def test_readBytes(self):
        self.reader.feed_data(b"\04\00\00\00abcd")
        data = yield from self.channel.readBytes()
        self.assertEqual(data, b"abcd")

    @async_tst
    def test_readHash(self):
        self.feedHash(Hash("a", 5))
        data = yield from self.channel.readHash()
        self.assertEqual(data["a"], 5)

    def test_writeHash(self):
        h = Hash("b", 7)
        self.channel.writeHash(h)
        self.assertChecksum(124256394)

    def test_writeSize(self):
        self.channel.writeSize(218)
        self.assertChecksum(57409755)

    @async_tst
    def test_nextChunk(self):
        future = Future()
        task = background(self.channel.nextChunk(future))
        yield from self.sleep()
        self.assertChecksum(1)
        future.set_result(self.sample_data)
        yield from self.sleep()
        self.assertChecksum(3020956469)
        self.assertFalse(task.done())
        self.feedHash(Hash("reason", "update"))
        yield from task

    @async_tst
    def test_nextChunk_eof(self):
        future = Future()
        task = background(self.channel.nextChunk(future))
        yield from self.sleep()
        self.reader.feed_eof()
        try:
            yield from task
        except IncompleteReadError as error:
            self.assertFalse(error.partial)
        else:
            self.fail()

    @coroutine
    def handler(self, data, meta):
        self.data.append(data)
        self.meta.append(meta)

    @coroutine
    def eos_handler(self, name):
        self.eos_channel = name

    def feedTestData(self):
        encoded = encodeBinary(Hash("a", 7))
        meta = Hash("source", "a", "timestamp", False)
        self.feedHash(Hash("byteSizes", [len(encoded)] * 3,
                      "sourceInfo", [meta] * 3))
        self.reader.feed_data(pack("<I", 3 * len(encoded)))
        for i in range(3):
            self.reader.feed_data(encoded)

    @async_tst
    def test_readChunk(self):
        network = NetworkInput({})
        network.handler = self.handler
        network.raw = False

        class Test(Proxy):
            a = Int32()

        task = background(network.readChunk(self.channel, Test))
        self.feedTestData()
        self.data = []
        self.meta = []
        self.assertTrue((yield from task))
        self.assertEqual(len(self.data), 3)
        self.assertEqual(self.data[1].a, 7)
        self.assertEqual(len(self.meta), 3)
        self.assertEqual(self.meta[2].source, "a")
        self.assertFalse(self.meta[0].timestamp)

    @async_tst
    def test_readChunk_raw(self):
        network = NetworkInput({})
        network.raw = True
        network.handler = self.handler
        task = background(network.readChunk(self.channel, None))
        self.feedTestData()
        self.data = []
        self.meta = []
        self.assertTrue((yield from task))
        self.assertEqual(len(self.data), 3)
        self.assertEqual(self.data[1]["a"], 7)
        self.assertEqual(len(self.meta), 3)
        self.assertEqual(self.meta[2].source, "a")
        self.assertFalse(self.meta[0].timestamp)

    @async_tst
    def test_readChunk_eos(self):
        network = NetworkInput({})
        network.raw = True
        network.handler = self.handler
        network.end_of_stream_handler = self.eos_handler
        self.assertEqual(self.eos_channel, "")
        task = background(network.readChunk(self.channel, None))
        self.feedHash(Hash("endOfStream", True))
        self.reader.feed_data(b"\0\0\0\0")
        self.data = []
        self.meta = []
        self.assertTrue((yield from task))
        self.assertEqual(self.data, [])
        self.assertEqual(self.eos_channel, "channelname")
        self.assertEqual(len(self.meta), 0)

    @async_tst
    def test_readChunk_eof(self):
        network = NetworkInput({})
        network.parent = Mock()
        self.reader.feed_eof()
        self.assertFalse((yield from network.readChunk(self.channel, None)))

    @coroutine
    def write_something(self, output):
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        for i in range(10):
            output.writeChunkNoWait(self.sample_data)
            yield from self.sleep()
        return task

    @async_tst
    def test_shared_queue(self):
        output = NetworkOutput({"noInputShared": "queue"})
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "queue", "instanceId", "Test"))
        task = yield from self.write_something(output)
        self.assertChecksum(3020956469)
        for checksum in [452019817, 881158557, 14388433, 2145693701]:
            self.feedRequest()
            yield from self.sleep()
            self.assertChecksum(checksum)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_shared_drop(self):
        output = NetworkOutput({"noInputShared": "drop"})
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "drop", "instanceId", "Test"))
        task = yield from self.write_something(output)
        yield from self.sleep()
        self.assertChecksum(3020956469)
        self.feedRequest()
        yield from self.sleep()
        self.feedRequest()
        for _ in range(100):
            self.assertChecksum(452019817)
            yield from self.sleep()
        output.writeChunkNoWait(self.sample_data)
        yield from self.sleep()
        self.assertChecksum(881158557)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_shared_throw(self):
        output = NetworkOutput({"noInputShared": "throw"})
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "throw", "instanceId", "Test"))
        yield from self.sleep()
        output.writeChunkNoWait(self.sample_data)
        yield from self.sleep()
        self.feedRequest()
        yield from self.sleep()
        output.writeChunkNoWait(self.sample_data)
        with self.assertRaises(QueueFull):
            output.writeChunkNoWait(self.sample_data)
            output.writeChunkNoWait(self.sample_data)
        self.assertChecksum(3020956469)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_shared_wait(self):
        output = NetworkOutput({"noInputShared": "wait"})
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "wait", "instanceId", "Test"))
        yield from self.sleep()
        yield from output.writeChunk([(Hash("a", 5), FakeTimestamp())])
        yield from self.sleep()
        self.feedRequest()
        yield from self.sleep()
        yield from output.writeChunk(self.sample_data)
        yield from output.writeChunk(self.sample_data)
        waiter = background(output.writeChunk(self.sample_data))
        yield from self.sleep()
        self.assertFalse(waiter.done())
        self.feedRequest()
        yield from self.sleep()
        self.assertTrue(waiter.done())
        self.assertChecksum(2194103602)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_copy_queue(self):
        output = NetworkOutput({"noInputShared": "drop"})
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "queue", "instanceId", "Test"))
        task = yield from self.write_something(output)
        self.assertChecksum(3020956469)
        for checksum in [452019817, 881158557, 14388433, 2145693701]:
            self.feedRequest()
            yield from self.sleep()
            self.assertChecksum(checksum)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_copy_drop(self):
        output = NetworkOutput({"noInputShared": "drop"})
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "drop", "instanceId", "Test"))
        task = yield from self.write_something(output)
        yield from self.sleep()
        self.assertChecksum(3020956469)
        self.feedRequest()
        yield from self.sleep()
        for _ in range(100):
            self.assertChecksum(3020956469)
            yield from self.sleep()
        output.writeChunkNoWait(self.sample_data)
        self.feedRequest()
        yield from self.sleep()
        self.assertChecksum(452019817)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_copy_throw(self):
        output = NetworkOutput({"noInputShared": "drop"})
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "throw", "instanceId", "Test"))
        output.writeChunkNoWait(self.sample_data)
        yield from self.sleep()
        output.writeChunkNoWait(self.sample_data)
        yield from self.sleep()
        output.writeChunkNoWait(self.sample_data)
        yield from self.sleep()
        with self.assertRaises(QueueFull):
            output.writeChunkNoWait(self.sample_data)
        self.assertChecksum(3020956469)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_copy_wait(self):
        output = NetworkOutput({"noInputShared": "drop"})
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "wait", "instanceId", "Test"))
        yield from output.writeChunk(self.sample_data)
        yield from self.sleep()
        yield from output.writeChunk(self.sample_data)
        self.feedRequest()
        yield from self.sleep()
        yield from output.writeChunk(self.sample_data)
        yield from output.writeChunk(self.sample_data)
        waiter = background(output.writeChunk(self.sample_data))
        yield from self.sleep()
        self.assertFalse(waiter.done())
        self.feedRequest()
        yield from self.sleep()
        self.assertTrue(waiter.done())
        self.assertChecksum(881158557)
        self.reader.feed_eof()
        yield from task
        self.assertTrue(self.writer.transport.closed)


if __name__ == "__main__":
    main()
