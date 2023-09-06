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

import threading
import time

import karabind
import pytest

import karathon

debugFlag = False
lock = threading.Lock()


@pytest.fixture(scope="module")
def event_loop():
    # We test the TCP code here and it does not matter which event loop
    # bindings we use - so choose karabind.
    EventLoop = karabind.EventLoop
    loop_thread = threading.Thread(target=EventLoop.work)
    loop_thread.start()

    yield  # now test is executed

    EventLoop.stop()
    loop_thread.join()


@pytest.mark.parametrize(
    "Connection, Hash, fullyEqual",
    [(karabind.Connection, karabind.Hash, karabind.fullyEqual),
     (karathon.Connection, karathon.Hash, karathon.fullyEqual)
     ])
def test_synch_write_read(event_loop, Connection, Hash, fullyEqual):
    # Alice as server, bob as client. Server cannot be started synchronously
    # since then the port that the client should connect to is not available.

    aliceConn = Connection.create("Tcp", Hash("type", "server"))
    alice = None  # to become the Channel

    def onConnect(ec, channel):
        nonlocal alice
        alice = channel

    aPort = aliceConn.startAsync(onConnect)

    bobCfg = Hash("type", "client", "hostname", "localhost", "port", aPort)
    bobConn = Connection.create("Tcp", bobCfg)
    bob = bobConn.start()  # bob is the Channel

    # Wait until alice took note of the connection and her channel is available
    timeout = 2
    while alice is None and timeout > 0:
        time.sleep(0.01)
        timeout -= 0.01

    assert alice is not None

    # Alice and Bob are prepared, so we start testing write and read methods.

    alice.write("messageStr")  # str
    readMsg = bob.readStr()
    assert readMsg == "messageStr"

    alice.write(b"messageBytes")  # bytes
    readMsg = bob.readStr()
    assert readMsg == "messageBytes"

    alice.write(bytearray("messageBytearray", "utf-8"))  # bytearray
    readMsg = bob.readStr()
    assert readMsg == "messageBytearray"

    alice.write(Hash("hash", "message"))  # Hash
    readMsg = bob.readHash()
    assert fullyEqual(readMsg, Hash("hash", "message"))

    if type(aliceConn) is not karathon.Connection:  # TODO FIXME
        # With karathon I get
        # TypeError: Python type in parameters is not supported
        # The above exception was the direct cause of the following exception:
        # <snip>
        #        with pytest.raises(RuntimeError) as excinfo:
        # >           alice.write(1)  # not supported
        # E           SystemError: <Boost.Python.function object at 0x...> \
        #                                  returned a result with an error set
        with pytest.raises(RuntimeError) as excinfo:
            alice.write(1)  # not supported to write int
            assert "Python Exception: Not supported type" in str(excinfo.value)

    alice.write(Hash("str", "header"), "messageStr")  # Hash, str
    readHeader, readMsg = bob.readHashStr()
    assert fullyEqual(readHeader, Hash("str", "header"))
    assert readMsg == "messageStr"

    alice.write(Hash("bytes", "header"), b"messageBytes")  # Hash, bytes
    readHeader, readMsg = bob.readHashStr()
    assert fullyEqual(readHeader, Hash("bytes", "header"))
    assert readMsg == "messageBytes"

    alice.write(Hash("bytearray", "header"),
                bytearray("messageBytearray", "utf-8"))  # bytearray
    readHeader, readMsg = bob.readHashStr()
    assert fullyEqual(readHeader, Hash("bytearray", "header"))
    assert readMsg == "messageBytearray"

    alice.write(Hash("a", "header"), Hash("hash", "message"))  # Hash, Hash
    readHeader, readMsg = bob.readHashHash()
    assert fullyEqual(readHeader, Hash("a", "header"))
    assert fullyEqual(readMsg, Hash("hash", "message"))

    if type(aliceConn) is not karathon.Connection:  # TODO FIXME
        # With karathon I get TypeError etc., as above
        with pytest.raises(RuntimeError) as excinfo:
            alice.write(Hash("a", "header"), 1)  # not supported
            assert "Python Exception: Not supported type" in str(excinfo.value)

        # Not sure which error we would get here with karathon
        with pytest.raises(TypeError) as excinfo:
            alice.write(1, "message")  # not supported header type
            assert ("TypeError:  write(): incompatible function arguments."
                    in str(excinfo.value))


def DEBUG(s):
    if debugFlag:
        lock.acquire()
        print(s)
        lock.release()


class Server:
    def __init__(self, Connection, Hash):
        # create connection object
        self.conn = Connection.create("Tcp", Hash("type", "server", "port", 0))
        # register connect handler for incoming connections
        self.port = self.conn.startAsync(self.onConnect)
        DEBUG(f"\nSRV listening port : {self.port}")

    def onError(self, ec, channel):
        DEBUG(f"SRV onError #{ec.value()} => {ec.message()}")
        DEBUG(f" -- close channel: id {id(channel)}")
        channel.close()
        DEBUG("SRV channel closed")

    def onConnect(self, ec, channel):
        DEBUG(f"SRV onConnect entry: #{ec.value()} -> {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        # register read handler for incoming Hash messages
        channel.readAsyncHashHash(self.onReadHashHash)
        DEBUG("SRV onConnect exit.")

    def onReadHashHash(self, ec, channel, h, b):
        DEBUG(f"SRV onReadHashHash: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        DEBUG(f"SRV onReadHashHash:\nHEADER ...\n{h}BODY...\n{b}\n")
        # process the message
        b["server"] = "SERVER: message processed!"
        # send back to client
        channel.writeAsyncHashHash(h, b, self.onWriteComplete)
        DEBUG("SRV onReadHashHash exit.")

    def onWriteComplete(self, ec, channel):
        DEBUG("SRV onWriteComplete entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        # register for reading new Hash message
        channel.readAsyncHashHash(self.onReadHashHash)
        DEBUG("SRV onWriteComplete exit.")

    # this method stops server
    def stop(self):
        self.conn.stop()


@pytest.mark.parametrize(
    "Connection, EventLoop, Hash",
    [
     (karabind.Connection, karabind.EventLoop, karabind.Hash),
     # (karathon.Connection, karathon.EventLoop, karathon.Hash)
     ])
def test_tcp_client_server(Connection, EventLoop, Hash):
    # Pass 'karathon' or 'karabind' versions of Connection, EventLoop and Hash
    server = Server(Connection, Hash)
    sthread = threading.Thread(target=EventLoop.run)
    sthread.start()
    EventLoop.addThread(4)
    DEBUG("CLN: EventLoop thread started...")

    body = None
    failed = None

    # define client callbacks ...

    def onError(ec, channel):
        channel.close()
        server.stop()
        # time.sleep(1)
        EventLoop.stop()

    def onReadHashHash(ec, channel, header, body):
        nonlocal failed
        DEBUG(f"CLN onReadHashHash #{ec.value()} => {ec.message()}")
        DEBUG(f"CLN onReadHashHash ...\nHEADER...\n{header}BODY...\n{body}\n")
        if ec.value() != 0:
            onError(ec, channel)
            return

        # Inspect received Hash ...
        try:
            assert len(body) == 4
            assert body['server'] == "SERVER: message processed!"
            assert body['a.b.c'] == 1
            assert body['x.y.z'] == [1, 2, 3, 4, 5]
            assert body['d.abc'] == 'rubbish'
        except BaseException as exc:
            failed = f'Hash inspection failed: {exc}'
        finally:
            channel.close()
            server.stop()
            DEBUG("CLN: EventLoop stop")
            EventLoop.stop()
            DEBUG("CLN onReadHashHash exit.")

    def onWriteComplete(ec, channel):
        DEBUG("CLN onWriteComplete entry.")
        if ec.value() != 0:
            onError(ec, channel)
            return

        with pytest.raises(RuntimeError):
            channel.readAsyncHashHash(None)

        channel.readAsyncHashHash(onReadHashHash)
        DEBUG("CLN onWriteComplete exit")

    def onConnect(ec, channel):
        nonlocal body
        DEBUG(f"CLN onConnect: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            onError(ec, channel)
            return

        h = Hash(
            "a.b.c", 1,
            "x.y.z", [1, 2, 3, 4, 5],
            "d", Hash("abc", "rubbish"))

        header = Hash("par1", 12)
        body = h  # keep object alive until write complete
        DEBUG("CLN onConnect writeAsyncHashHash:\nHEADER...")
        DEBUG(f"{header}BODY\n{body}\n")

        with pytest.raises(RuntimeError):
            channel.writeAsyncHashHash(header, body)

        channel.writeAsyncHashHash(header, body, onWriteComplete)
        DEBUG("CLN onConnect exit.")

    # Asynchronous TCP client
    # create client connection object
    DEBUG("CLN create Connection and start it")
    client = Connection.create(
        "Tcp", Hash("type", "client", "hostname", "localhost",
                    "port", server.port))

    with pytest.raises(RuntimeError):
        client.startAsync()

    client.startAsync(onConnect)

    sthread.join()

    assert failed is None

    DEBUG("CLN EXIT")
