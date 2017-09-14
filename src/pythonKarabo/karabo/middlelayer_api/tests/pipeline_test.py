from asyncio import coroutine, StreamReader, StreamWriter, WriteTransport
from unittest import main
from unittest.mock import Mock
from struct import pack
from zlib import adler32

from karabo.middlelayer import background, encodeBinary, Hash, Int32, Proxy
from karabo.middlelayer_api.pipeline import Channel, NetworkInput

from .eventloop import DeviceTest, async_tst


class ChecksumTransport(WriteTransport):
    def __init__(self):
        self.checksum = adler32(b"")

    def write(self, data):
        self.checksum = adler32(data, self.checksum)


class TestChannel(DeviceTest):
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

    @coroutine
    def handler(self, data, meta):
        self.data.append(data)
        self.meta.append(meta)

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
        task = background(network.readChunk(self.channel, None))
        self.feedHash(Hash("endOfStream", True))
        self.reader.feed_data(b"\0\0\0\0")
        self.data = []
        self.meta = []
        self.assertTrue((yield from task))
        self.assertEqual(self.data, [None])
        self.assertEqual(len(self.meta), 1)
        self.assertEqual(self.meta[0].source, "channelname")
        self.assertFalse(self.meta[0].timestamp)

    @async_tst
    def test_readChunk_eof(self):
        network = NetworkInput({})
        network.parent = Mock()
        self.reader.feed_eof()
        self.assertFalse((yield from network.readChunk(self.channel, None)))


if __name__ == "__main__":
    main()

