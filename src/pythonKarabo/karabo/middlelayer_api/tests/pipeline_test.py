from asyncio import (
    Future, get_event_loop, IncompleteReadError, sleep, StreamReader,
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
        loop = get_event_loop()
        self.reader = StreamReader(loop=loop)
        self.writer = StreamWriter(ChecksumTransport(), None, None, loop=loop)
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

    async def sleep(self):
        for _ in range(10):
            await sleep(0)

    @async_tst
    async def test_readBytes(self):
        self.reader.feed_data(b"\04\00\00\00abcd")
        data = await self.channel.readBytes()
        self.assertEqual(data, b"abcd")

    @async_tst
    async def test_readHash(self):
        self.feedHash(Hash("a", 5))
        data = await self.channel.readHash()
        self.assertEqual(data["a"], 5)

    def test_writeHash(self):
        h = Hash("b", 7)
        self.channel.writeHash(h)
        self.assertChecksum(124256394)

    def test_writeSize(self):
        self.channel.writeSize(218)
        self.assertChecksum(57409755)

    @async_tst
    async def test_nextChunk(self):
        future = Future()
        task = background(self.channel.nextChunk(future))
        await self.sleep()
        self.assertChecksum(1)
        future.set_result(self.sample_data)
        await self.sleep()
        self.assertChecksum(3020956469)
        self.assertFalse(task.done())
        self.feedHash(Hash("reason", "update"))
        await task

    @async_tst
    async def test_nextChunk_eof(self):
        future = Future()
        task = background(self.channel.nextChunk(future))
        await self.sleep()
        self.reader.feed_eof()
        try:
            await task
        except IncompleteReadError as error:
            self.assertFalse(error.partial)
        else:
            self.fail()

    async def handler(self, data, meta):
        self.data.append(data)
        self.meta.append(meta)

    async def eos_handler(self, name):
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
    async def test_readChunk(self):
        network = NetworkInput({})
        network.handler = self.handler
        network.raw = False
        await network._run()

        class Test(Proxy):
            a = Int32()

        task = background(network.readChunk(self.channel, Test))
        self.feedTestData()
        self.data = []
        self.meta = []
        self.assertTrue((await task))
        self.assertEqual(len(self.data), 3)
        self.assertEqual(self.data[1].a, 7)
        self.assertEqual(len(self.meta), 3)
        self.assertEqual(self.meta[2].source, "a")
        self.assertFalse(self.meta[0].timestamp)

    @async_tst
    async def test_readChunk_raw(self):
        network = NetworkInput({})
        network.raw = True
        network.handler = self.handler
        await network._run()
        task = background(network.readChunk(self.channel, None))
        self.feedTestData()
        self.data = []
        self.meta = []
        self.assertTrue((await task))
        self.assertEqual(len(self.data), 3)
        self.assertEqual(self.data[1]["a"], 7)
        self.assertEqual(len(self.meta), 3)
        self.assertEqual(self.meta[2].source, "a")
        self.assertFalse(self.meta[0].timestamp)

    @async_tst
    async def test_readChunk_eos(self):
        network = NetworkInput({})
        network.raw = True
        network.handler = self.handler
        network.end_of_stream_handler = self.eos_handler
        await network._run()
        self.assertEqual(self.eos_channel, "")
        task = background(network.readChunk(self.channel, None))
        self.feedHash(Hash("endOfStream", True))
        self.reader.feed_data(b"\0\0\0\0")
        self.data = []
        self.meta = []
        self.assertTrue((await task))
        self.assertEqual(self.data, [])
        self.assertEqual(self.eos_channel, "channelname")
        self.assertEqual(len(self.meta), 0)

    @async_tst
    async def test_readChunk_eof(self):
        network = NetworkInput({})
        network.parent = Mock()
        await network._run()
        self.reader.feed_eof()
        self.assertFalse((await network.readChunk(self.channel, None)))

    async def write_something(self, output, number=10):
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        for i in range(number):
            output.writeChunkNoWait(self.sample_data)
            await self.sleep()
        return task

    @async_tst
    async def test_shared_queue(self):
        output = NetworkOutput({"noInputShared": "queue"})
        # Initialize
        await output._run()
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "queue", "instanceId", "Test"))
        task = await self.write_something(output)
        self.assertChecksum(3020956469)
        for checksum in [452019817, 881158557, 14388433, 2145693701]:
            self.feedRequest()
            await self.sleep()
            self.assertChecksum(checksum)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_shared_drop(self):
        output = NetworkOutput({"noInputShared": "drop"})
        await output._run()
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "drop", "instanceId", "Test"))
        task = await self.write_something(output)
        await self.sleep()
        self.assertChecksum(3020956469)
        self.feedRequest()
        await self.sleep()
        self.feedRequest()
        for _ in range(100):
            self.assertChecksum(452019817)
            await self.sleep()
        output.writeChunkNoWait(self.sample_data)
        await self.sleep()
        self.assertChecksum(881158557)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_shared_queue_drop(self):
        output = NetworkOutput({"noInputShared": "queueDrop"})
        await output._run()
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "queueDrop", "instanceId", "Test"))
        task = await self.write_something(output, 200)
        await self.sleep()
        self.assertChecksum(3020956469)
        self.feedRequest()
        await self.sleep()
        self.feedRequest()
        await self.sleep()
        for _ in range(100):
            self.assertChecksum(881158557)
            await self.sleep()
        output.writeChunkNoWait(self.sample_data)
        await self.sleep()
        self.assertChecksum(881158557)
        for i in range(200):
            output.writeChunkNoWait(self.sample_data)
        await self.sleep()
        self.assertChecksum(881158557)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    def test_shared_throw(self):
        with self.assertRaises(ValueError):
            output = NetworkOutput({"noInputShared": "throw"})  # noqa

    @async_tst
    async def test_shared_wait(self):
        output = NetworkOutput({"noInputShared": "wait"})
        output.channelName = "channelname"
        await output._run()
        task = background(output.serve(self.reader, self.writer))
        self.feedHash(Hash("reason", "hello", "dataDistribution", "shared",
                           "onSlowness", "wait", "instanceId", "Test"))
        await self.sleep()
        await output.writeChunk([(Hash("a", 5), FakeTimestamp())])
        await self.sleep()
        self.feedRequest()
        await self.sleep()
        await output.writeChunk(self.sample_data)
        await output.writeChunk(self.sample_data)
        waiter = background(output.writeChunk(self.sample_data))
        await self.sleep()
        self.assertFalse(waiter.done())
        self.feedRequest()
        await self.sleep()
        self.assertTrue(waiter.done())
        self.assertChecksum(2194103602)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_copy_queue(self):
        output = NetworkOutput({"noInputShared": "drop"})
        await output._run()
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "queue", "instanceId", "Test"))
        task = await self.write_something(output)
        self.assertChecksum(3020956469)
        for checksum in [452019817, 881158557, 14388433, 2145693701]:
            self.feedRequest()
            await self.sleep()
            self.assertChecksum(checksum)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_copy_drop(self):
        output = NetworkOutput({"noInputShared": "drop"})
        await output._run()
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "drop", "instanceId", "Test"))
        task = await self.write_something(output)
        await self.sleep()
        self.assertChecksum(3020956469)
        self.feedRequest()
        await self.sleep()
        for _ in range(100):
            self.assertChecksum(3020956469)
            await self.sleep()
        output.writeChunkNoWait(self.sample_data)
        self.feedRequest()
        await self.sleep()
        self.assertChecksum(452019817)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_copy_queue_drop(self):
        output = NetworkOutput({"noInputShared": "drop"})
        await output._run()
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "queueDrop", "instanceId", "Test"))
        task = await self.write_something(output, 200)
        await self.sleep()
        self.assertChecksum(3020956469)
        # Write like crazy, we do not have a request and roll!
        output.writeChunkNoWait(self.sample_data)
        output.writeChunkNoWait(self.sample_data)
        output.writeChunkNoWait(self.sample_data)
        self.assertChecksum(3020956469)
        self.feedRequest()
        await self.sleep()
        for _ in range(100):
            self.assertChecksum(452019817)
            await self.sleep()
        output.writeChunkNoWait(self.sample_data)
        self.feedRequest()
        await self.sleep()
        self.assertChecksum(881158557)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_copy_throw(self):
        output = NetworkOutput({"noInputShared": "drop"})
        await output._run()
        output.channelName = "channelname"
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "throw", "instanceId", "Test"))
        task = await self.write_something(output)
        await self.sleep()
        self.assertChecksum(3020956469)
        self.feedRequest()
        await self.sleep()
        for _ in range(100):
            self.assertChecksum(3020956469)
            await self.sleep()
        output.writeChunkNoWait(self.sample_data)
        self.feedRequest()
        await self.sleep()
        self.assertChecksum(452019817)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)

    @async_tst
    async def test_copy_wait(self):
        output = NetworkOutput({"noInputShared": "drop"})
        await output._run()
        output.channelName = "channelname"
        task = background(output.serve(self.reader, self.writer))
        self.feedHash(Hash("reason", "hello", "dataDistribution", "copy",
                           "onSlowness", "wait", "instanceId", "Test"))
        await output.writeChunk(self.sample_data)
        await self.sleep()
        await output.writeChunk(self.sample_data)
        self.feedRequest()
        await self.sleep()
        await output.writeChunk(self.sample_data)
        await output.writeChunk(self.sample_data)
        waiter = background(output.writeChunk(self.sample_data))
        await self.sleep()
        self.assertFalse(waiter.done())
        self.feedRequest()
        await self.sleep()
        self.assertTrue(waiter.done())
        self.assertChecksum(881158557)
        self.reader.feed_eof()
        await task
        self.assertTrue(self.writer.transport.closed)


if __name__ == "__main__":
    main()
