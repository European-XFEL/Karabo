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

import karabind
import pytest

# import karathon

debugFlag = False
lock = threading.Lock()


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
