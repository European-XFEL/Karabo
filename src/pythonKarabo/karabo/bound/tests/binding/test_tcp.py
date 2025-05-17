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

import pytest

from karabo.bound import (
    Broker, Connection, EventLoop, Hash, Schema, SignalSlotable, fullyEqual)

debugFlag = False
lock = threading.Lock()

# For debugging to get exceptions printed that are caugth by C++ event loop:
# from karabo.bound import Logger
# Logger.configure()
# Logger.useConsole()


@pytest.fixture(scope="module")
def eventLoop():
    loop_thread = threading.Thread(target=EventLoop.work)
    loop_thread.start()
    # test_asynch_write_read needs an extra thread if readAsyncXxx would be
    # called before writeAsyncXxx
    EventLoop.addThread()

    yield  # now all (since scope="module") tests are executed

    EventLoop.stop()
    loop_thread.join(timeout=10)
    assert not loop_thread.is_alive()


def test_broker():

    s = Schema()
    Broker.expectedParameters(s)
    assert s.has("brokers")

    brokerType = Broker.brokerTypeFromEnv()
    assert brokerType in Broker.getRegisteredClasses()

    # Create a Broker via SignalSlotable to test the only (besides from some
    # macro) non-static member function that is exposed to Python:
    sigSlot = SignalSlotable("brokerCreator", Hash(), 60, Hash())
    broker = sigSlot.getConnection()
    assert type(broker) is Broker
    brokerProtocol = brokerType
    assert broker.getBrokerUrl().startswith(brokerProtocol + "://")


# Could become a fixture that requires eventLoop fixture, but then I do not
# know how to pass the classes
def setup_server_client():
    """
    Create server and client connected to each other, using Connection and Hash
    classes as passed by argument.
    """
    # Alice as server, bob as client. Server cannot be started synchronously
    # since then the port that the client should connect to is not available.

    aliceConn = Connection.create("Tcp", Hash("type", "server"))
    alice = None  # to become the Channel

    connectedCond = threading.Condition()

    def onConnect(ec, channel):
        nonlocal alice
        alice = channel
        with connectedCond:
            connectedCond.notify()

    aPort = aliceConn.startAsync(onConnect)

    bobCfg = Hash("type", "client", "hostname", "localhost", "port", aPort)
    bobConn = Connection.create("Tcp", bobCfg)
    with connectedCond:
        bob = bobConn.start()  # bob is the Channel
        connectedCond.wait(timeout=10)  # wait until notified

    assert alice is not None
    aliceConn2 = alice.getConnection()
    assert type(aliceConn) is Connection
    assert aliceConn2 is aliceConn

    # Better return (and thus keep alive) the connections as well:
    # C++ TcpChannel keeps only weak_ptr to its TcpConnection.
    return alice, aliceConn, bob, bobConn


def test_synch_write_read(eventLoop):

    alice, aliceConn, bob, bobConn = setup_server_client()

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

    with pytest.raises(RuntimeError) as excinfo:
        alice.write(Hash("a", "header"), 1)  # not supported
        assert "Python Exception: Not supported type" in str(excinfo.value)

    with pytest.raises(TypeError) as excinfo:
        alice.write(1, "message")  # not supported header type
        assert ("TypeError:  write(): incompatible function arguments."
                in str(excinfo.value))


def test_asynch_write_read(eventLoop):
    # Test asynchronous write and read methods.

    alice, aliceConn, bob, bobConn = setup_server_client()

    ###################################################################
    # Write and read str
    ###################################################################

    ecWrite = None
    channelWrite = None
    conditionWrite = threading.Condition()

    def writeComplete(ec, channel):
        nonlocal ecWrite, channelWrite
        ecWrite = ec
        channelWrite = channel
        with conditionWrite:
            conditionWrite.notify()

    alice.writeAsyncStr("messageStr", writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    ecRead = None
    channelRead = None
    msgRead = None
    conditionRead = threading.Condition()

    def onRead1(ec, channel, msg):
        nonlocal ecRead, channelRead, msgRead
        ecRead = ec
        channelRead = channel
        msgRead = msg
        with conditionRead:
            conditionRead.notify()

    bob.readAsyncStr(onRead1)
    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert msgRead == "messageStr"

    ###################################################################
    # Write bytes (and read as str)
    ###################################################################

    # Reset to re-use writeComplete and onRead1
    ecWrite = channelWrite = None
    ecRead = channelRead = msgRead = None

    alice.writeAsyncStr(b"messageStr", writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncStr(onRead1)
    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert msgRead == "messageStr"

    ###################################################################
    # Write bytearray (and read as str)
    ###################################################################
    # Reset to re-use writeComplete and onRead1
    ecWrite = channelWrite = ecRead = None
    channelRead = msgRead = None

    alice.writeAsyncStr(bytearray("messageStr", encoding="utf8"),
                        writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncStr(onRead1)

    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert msgRead == "messageStr"

    ###################################################################
    # Write and read Hash
    ###################################################################
    # Reset to re-use writeComplete and onRead1
    ecWrite = channelWrite = None
    ecRead = channelRead = msgRead = None

    alice.writeAsyncHash(Hash("hash", "message"), writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncHash(onRead1)
    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert fullyEqual(msgRead, Hash("hash", "message"))

    ###################################################################
    # Write and read Hash, string
    ###################################################################

    # Reset to re-use writeComplete
    ecWrite = channelWrite = None

    # Need an onRead2 for header/body messages
    ecRead = channelRead = headerRead = msgRead = None

    def onRead2(ec, channel, header, msg):
        nonlocal ecRead, channelRead, headerRead, msgRead
        ecRead = ec
        channelRead = channel
        headerRead = header
        msgRead = msg
        with conditionRead:
            conditionRead.notify()

    alice.writeAsyncHashStr(Hash("header", "message"), "body",
                            writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncHashStr(onRead2)

    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert fullyEqual(headerRead, Hash("header", "message"))
    assert msgRead == "body"

    ###################################################################
    # Write and read Hash, bytes
    ###################################################################
    # Reset to re-use writeComplete and onRead2
    ecWrite = channelWrite = None
    ecRead = channelRead = headerRead = msgRead = None

    alice.writeAsyncHashStr(Hash("header", "message b"), b"bytes",
                            writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncHashStr(onRead2)

    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert fullyEqual(headerRead, Hash("header", "message b"))
    assert msgRead == "bytes"

    ###################################################################
    # Write and read Hash, bytearray
    ###################################################################
    # Reset to re-use writeComplete and onRead2
    ecWrite = channelWrite = None
    ecRead = channelRead = headerRead = msgRead = None

    alice.writeAsyncHashStr(Hash("header", "message b2"),
                            bytearray("bytearray", encoding="utf8"),
                            writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncHashStr(onRead2)

    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert fullyEqual(headerRead, Hash("header", "message b2"))
    assert msgRead == "bytearray"

    ###################################################################
    # Write and read Hash, Hash
    ###################################################################
    # Reset to re-use writeComplete and onRead2
    ecWrite = channelWrite = None
    ecRead = channelRead = headerRead = msgRead = None

    alice.writeAsyncHashHash(Hash("header", "message a"),
                             Hash("body", "message b"),
                             writeComplete)
    with conditionWrite:
        conditionWrite.wait(timeout=10)
    assert ecWrite.value() == 0
    assert alice is channelWrite

    bob.readAsyncHashHash(onRead2)

    with conditionRead:
        conditionRead.wait(timeout=10)
    assert ecRead.value() == 0
    assert bob is channelRead
    assert fullyEqual(headerRead, Hash("header", "message a"))
    assert fullyEqual(msgRead, Hash("body", "message b"))


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
        channel.close()
        DEBUG("SRV channel closed")

    def onWriteHashHashComplete(self, ec, channel):
        DEBUG("SRV onWriteHashComplete entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # register for reading new Hash/Hash message
        channel.readAsyncHashHash(self.onReadHashHash)
        DEBUG("SRV onWriteHashHashComplete exit.")

    def onReadHashHash(self, ec, channel, h, b):
        DEBUG(f"SRV onReadHashHash entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # process the message
        b["server"] = "SERVER: message processed!"
        # send back to client
        channel.writeAsyncHashHash(h, b, self.onWriteHashHashComplete)
        DEBUG("SRV onReadHashHash exit.")

    def onWriteHashStrComplete(self, ec, channel):
        DEBUG("SRV onWriteHashStrComplete entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # register for reading new Hash/Hash message
        channel.readAsyncHashHash(self.onReadHashHash)
        DEBUG("SRV onWriteHashStrComplete exit.")

    def onReadHashStr(self, ec, channel, h, s):
        DEBUG(f"SRV onReadHashStr entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # send back to client
        channel.writeAsyncHashStr(h, s, self.onWriteHashStrComplete)
        DEBUG("SRV onReadHashStr exit.")

    def onWriteHashComplete(self, ec, channel):
        DEBUG("SRV onWriteHashComplete entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # register for reading new Hash/Str message
        channel.readAsyncHashStr(self.onReadHashStr)
        DEBUG("SRV onWriteHashComplete exit.")

    def onReadHash(self, ec, channel, h):
        DEBUG(f"SRV onReadHash entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # send back to client
        channel.writeAsyncHash(h, self.onWriteHashComplete)
        DEBUG("SRV onReadHash exit.")

    def onWriteStrComplete(self, ec, channel):
        DEBUG("SRV onWriteStrComplete entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # register for reading new Hash message
        channel.readAsyncHash(self.onReadHash)
        DEBUG("SRV onWriteStrComplete exit.")

    def onReadStr(self, ec, channel, s):
        DEBUG(f"SRV onReadStr entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        channel.writeAsyncStr(s, self.onWriteStrComplete)
        DEBUG("SRV onReadStr exit.")

    def onConnect(self, ec, channel):
        DEBUG(f"SRV onConnect entry: #{ec.value()} -> {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # register read handler for incoming Str messages
        channel.readAsyncStr(self.onReadStr)
        DEBUG("SRV onConnect exit.")

    # this method stops server
    def stop(self):
        self.conn.stop()


def test_tcp_client_server():
    counter = 0
    count_max = 10
    server = Server(Connection, Hash)
    sthread = threading.Thread(target=EventLoop.run)
    sthread.start()
    EventLoop.addThread(4)
    DEBUG("CLN: EventLoop thread started...")

    failed = None

    def onError(ec, channel):
        channel.close()
        server.stop()
        EventLoop.stop()

    def onReadHashHash(ec, channel, header, body):
        nonlocal failed
        nonlocal counter
        counter += 1
        DEBUG(f"CLN onReadHashHash entry: #{ec.value()} => {ec.message()}")
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

        if counter >= count_max:
            channel.close()
            server.stop()
            DEBUG("CLN: EventLoop stop")
            EventLoop.stop()
        else:
            # Next message ...
            body = Hash(
                "a.b.c", 1,
                "x.y.z", [1, 2, 3, 4, 5],
                "d", Hash("abc", "rubbish"))
            header = Hash("par1", 12)
            channel.writeAsyncHashHash(header, body, onWriteHashHashComplete)
        DEBUG(f"CLN onReadHashHash exit.\t\tCOUNT = {counter}\n")

    def onWriteHashHashComplete(ec, channel):
        DEBUG("CLN onWriteHashHashComplete entry.")
        if ec.value() != 0:
            onError(ec, channel)
            return

        with pytest.raises(RuntimeError):
            channel.readAsyncHashHash(None)

        channel.readAsyncHashHash(onReadHashHash)
        DEBUG("CLN onWriteHashHashComplete exit")

    def onReadHashStr(ec, channel, h, s):
        nonlocal failed
        nonlocal counter
        counter += 1
        DEBUG(f"CLN onReadHashStr entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            onError(ec, channel)
            return

        try:
            assert s == "some message"
        except BaseException as exc:
            failed = f'Message inspection failed: {exc}'
            return

        # Next message ...
        body = Hash(
            "a.b.c", 1,
            "x.y.z", [1, 2, 3, 4, 5],
            "d", Hash("abc", "rubbish"))
        header = Hash("par1", 12)

        with pytest.raises(RuntimeError):
            channel.writeAsyncHashHash(header, body, None)

        channel.writeAsyncHashHash(header, body, onWriteHashHashComplete)
        DEBUG(f"CLN onReadHashStr exit.\t\t\tCOUNT = {counter}\n")

    def onWriteHashStrComplete(ec, channel):
        DEBUG("CLN onWriteHashStrComplete entry.")
        if ec.value() != 0:
            onError(ec, channel)
            return

        with pytest.raises(RuntimeError):
            channel.readAsyncHashStr(None)

        channel.readAsyncHashStr(onReadHashStr)
        DEBUG("CLN onWriteHashStrComplete exit")

    def onReadHash(ec, channel, body):
        nonlocal failed
        nonlocal counter
        counter += 1
        DEBUG(f"CLN onReadHash entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            onError(ec, channel)
            return

        try:
            assert len(body) == 1
            assert body['a'] == 12
        except BaseException as exc:
            failed = f'Message inspection failed: {exc}'
            return

        h = Hash('a', 1)
        s = "some message"
        channel.writeAsyncHashStr(h, s, onWriteHashStrComplete)
        DEBUG(f"CLN onReadHash exit.\t\t\tCOUNT = {counter}\n")

    def onWriteHashComplete(ec, channel):
        DEBUG("CLN onWriteHashComplete entry.")
        if ec.value() != 0:
            onError(ec, channel)
            return

        with pytest.raises(RuntimeError):
            channel.readAsyncHash(None)

        channel.readAsyncHash(onReadHash)
        DEBUG("CLN onWriteHashComplete exit")

    def onReadStr(ec, channel, s):
        nonlocal failed
        nonlocal counter
        counter += 1
        DEBUG(f"CLN onReadStr entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            onError(ec, channel)
            return

        try:
            assert s == 'Hello'
        except BaseException as exc:
            failed = f'Message inspection failed: {exc}'
            return

        h = Hash("a", 12)
        channel.writeAsyncHash(h, onWriteHashComplete)
        DEBUG(f"CLN onReadStr exit.\t\t\tCOUNT = {counter}\n")

    def onWriteStrComplete(ec, channel):
        DEBUG("CLN onWriteStrComplete entry.")
        if ec.value() != 0:
            onError(ec, channel)
            return

        with pytest.raises(RuntimeError):
            channel.readAsyncStr(None)

        channel.readAsyncStr(onReadStr)
        DEBUG("CLN onWriteStrComplete exit")

    def onConnect(ec, channel):
        nonlocal counter
        counter = 0
        DEBUG(f"CLN onConnect entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            onError(ec, channel)
            return

        with pytest.raises(RuntimeError):
            channel.writeAsyncStr("Tschüß", None)

        channel.writeAsyncStr("Hello", onWriteStrComplete)
        DEBUG(f"CLN onConnect exit.\t\t\tCOUNT = {counter}\n")

    # Asynchronous TCP client
    # create client connection object
    DEBUG("CLN create Connection and start it")
    client = Connection.create(
        "Tcp", Hash("type", "client", "hostname", "localhost",
                    "port", server.port))

    with pytest.raises(RuntimeError):
        client.startAsync(None)

    client.startAsync(onConnect)

    sthread.join()

    assert failed is None

    DEBUG("CLN EXIT")
