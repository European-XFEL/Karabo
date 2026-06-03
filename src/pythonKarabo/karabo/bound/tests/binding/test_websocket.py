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
from time import sleep

import pytest

from karabo.bound import Connection, Hash, Logger
from karabo.bound.testing import eventLoop  # noqa

debugFlag = False
lock = threading.Lock()

# For debugging to get exceptions printed that are caugth by C++ event loop:
Logger.configure()
Logger.useConsole()


def DEBUG(s):
    if debugFlag:
        lock.acquire()
        print(s)
        lock.release()


class Server:
    def __init__(self):
        # create connection object
        self.conn = Connection.create("WebSocket", Hash("type", "server",
                                                        "port", 8077))
        # register connect handler for incoming connections
        self.port = self.conn.startAsync(self.onConnect)
        DEBUG(f"\nSRV listening port : {self.port}")

    def onError(self, ec, channel):
        DEBUG(f"SRV onError #{ec.value()} => {ec.message()}")
        channel.close()
        DEBUG("SRV channel closed")

    def onReadHash(self, ec, channel, h):
        DEBUG(f"SRV onReadHash entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # send back to client
        channel.writeAsync(h)
        channel.readAsyncHash(self.onReadHash)
        DEBUG(f"SRV onReadHash continue. \t\t\th['a'] = {h['a']}")

    def onConnect(self, ec, channel):
        DEBUG(f"SRV onConnect entry: #{ec.value()} -> {ec.message()}")
        if ec.value() != 0:
            self.onError(ec, channel)
            return
        # register read handler for incoming Str messages
        channel.readAsyncHash(self.onReadHash)
        DEBUG("SRV onConnect exit.")

    # this method stops server
    def stop(self):
        self.conn.stop()


def test_websocket_client_server(eventLoop):  # noqa
    counter = 0
    count_max = 10
    server = Server()
    DEBUG("CLN: Server started...")

    failed = None
    done = False

    def onError(ec, channel):
        nonlocal done
        DEBUG("CLN: onError")
        channel.close()
        server.stop()
        done = True

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
            assert body['a'] == counter
        except BaseException as exc:
            failed = f'Message inspection failed: {exc}'
            return

        if body["a"] <= 10:
            channel.readAsyncHash(onReadHash)
            DEBUG(f"CLN onReadHash continue.\t\t\tCOUNT = {counter}\n")
            return
        DEBUG(f"CLN onReadHash EXIT.\t\t\tCOUNT = {counter}\n")

    def onConnect(ec, channel):
        nonlocal counter
        counter = 0
        DEBUG(f"CLN onConnect entry: #{ec.value()} => {ec.message()}")
        if ec.value() != 0:
            onError(ec, channel)
            return
        for i in range(1, 11):
            h = Hash("a", i)
            channel.writeAsync(h)
        channel.readAsyncHash(onReadHash)
        DEBUG("CLN onConnect exit.\n")

    # Asynchronous WebSocket client
    # create client connection object
    DEBUG("CLN create Connection and start it")
    client = Connection.create(
        "WebSocket", Hash("type", "client", "hostname", "localhost",
                          "port", server.port))
    with pytest.raises(RuntimeError):
        client.startAsync(None)
    client.startAsync(onConnect)
    DEBUG("CLN started Connection")

    # Wait until 'done' (could be done nice with condition variable or so...)
    timeout = 1
    while timeout > 0:
        if done or failed is not None:
            break
        timeout -= 0.01
        sleep(0.01)

    assert failed is None
    assert counter == count_max

    DEBUG("CLN EXIT")
