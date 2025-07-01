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
from asyncio import Queue, ensure_future, open_connection
from struct import calcsize, pack, unpack

from karabo.middlelayer import Hash, decodeBinary, encodeBinary


class GuiAdapter:

    def __init__(self, host, port):
        self.read_queue = Queue()
        self.writer = None
        self.host = host
        self.port = port
        self.reader = None
        self.read_loop = None
        self.connected = False

    async def login(self):
        loginInfo = Hash("type", "login")
        loginInfo["username"] = 'expert'
        loginInfo["password"] = ''
        loginInfo["provider"] = 'LOCAL'
        loginInfo["host"] = 'localhost'
        loginInfo["sessionToken"] = ''
        loginInfo["pid"] = os.getpid()
        loginInfo["version"] = '42.0.0'
        self.tcpWriteHash(loginInfo)
        await self.writer.drain()

    def tcpWriteHash(self, h: Hash):
        dataBytes = encodeBinary(h)
        self.writer.write(pack('I', len(dataBytes)))
        self.writer.write(dataBytes)

    def decode_message(self, data: bytes):
        h = decodeBinary(data)
        try:
            msg_type = h['type']
        except KeyError:
            print(f"Unknown message received: {h}")
        else:
            self.read_queue.put_nowait({"type": msg_type, "value": h})

    async def connect(self):
        if not self.connected:
            self.connected = True
            self.reader, self.writer = await open_connection(
                self.host, self.port)
            self.read_loop = ensure_future(self.reader_loop())

    async def get_next(self, msg_type: str):
        while True:
            item = await self.read_queue.get()
            if not item.get("type") == msg_type:
                continue
            return item["value"]

    async def disconnect(self):
        self.connected = False
        if self.read_loop is not None:
            self.read_loop.cancel()

    async def reader_loop(self):
        sizeFormat = 'I'  # unsigned int
        bytesNeededSize = calcsize(sizeFormat)
        while True:
            raw = await self.reader.readexactly(bytesNeededSize)
            bytes2read = unpack(sizeFormat, raw)[0]
            data = await self.reader.readexactly(bytes2read)
            self.decode_message(data)
