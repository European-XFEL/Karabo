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
from asyncio import (
    FIRST_COMPLETED, Queue, create_task, ensure_future, get_running_loop,
    sleep, wait)
from struct import pack, unpack

from websockets.asyncio.client import connect
from websockets.exceptions import ConnectionClosed

from karabo.middlelayer import Hash, decodeBinary, encodeBinary

DEBUG = True


class WebsocketsAdapter:

    def __init__(self, host, port):
        self.read_queue = Queue()
        self.client = None
        self.uri = f"ws://{host}:{port}"
        self.loop = get_running_loop()
        self.connected = False

    async def login(self, info: Hash | None = None):
        if self.connected:
            await self.disconnect()
        await self.connect()

        if info is None:
            info = Hash("type", "login")
            info["username"] = 'expert'
            info["password"] = ''
            info["host"] = 'localhost'
            info["sessionToken"] = ''
            info["pid"] = os.getpid()
            info["version"] = '42.0.0'

        await self.send(info)
        await sleep(0.2)

    async def send(self, h: Hash):
        data = encodeBinary(h)
        if self.client is not None:
            message = b''.join((pack('I', len(data)), data))
            await self.client.send(message)

    async def reset(self):
        while not self.read_queue.empty():
            self.read_queue.get_nowait()
            self.read_queue.task_done()

    async def connect(self):
        self.client = await connect(self.uri)
        if self.client is None:
            print("websocket is None! Exit")
            return
        self.connected = True
        self.read_loop = ensure_future(self.reader_loop())

    async def get_next(self, msg_type: str) -> Hash:
        while True:
            item = await self.read_queue.get()
            if not item.get("type") == msg_type:
                continue
            return item["value"]

    def get_all(self, msg_type: str) -> list[Hash]:
        """Empty the reading queue, returning all messages of msg_type"""
        ret = []
        while not self.read_queue.empty():
            item = self.read_queue.get_nowait()
            self.read_queue.task_done()
            if item.get("type") == msg_type:
                ret.append(item["value"])
        return ret

    async def disconnect(self):
        if self.connected:
            if self.client is not None:
                await self.client.close()
                self.client = None
            self.connected = False
        if self.read_loop is not None:
            self.read_loop.cancel()
            while not self.read_loop.done():
                await sleep(0.2)
            self.read_loop = None

    async def get_message(self):
        # Two tasks in parallel:
        # - reading a message from remote and
        # - waiting for possible 'close' frame from remote peer
        recv_task = create_task(self.client.recv())
        closed_task = create_task(self.client.wait_closed())
        background_tasks = [recv_task, closed_task]

        done, pending = await wait(background_tasks,
                                   return_when=FIRST_COMPLETED)
        for task in pending:
            task.cancel()

        if closed_task in done:
            return b''

        return recv_task.result()

    async def reader_loop(self):

        def process(message: bytes):
            h = decodeBinary(message)
            try:
                msg_type = h['type']
            except KeyError:
                print(f"Unknown message received: {h}")
            else:
                self.read_queue.put_nowait({"type": msg_type, "value": h})

        while True:
            try:
                message = await self.get_message()
                if message == b'':
                    break
                size = unpack('I', message[0:4])[0]
                data = message[4:]
                assert size == len(data), f'Assertion: {size} != {len(data)}'
                process(data)
            except ConnectionClosed:
                break

        # call 'disconnect' via eventloop since 'disconnect' cancels this task
        if self.connected:
            self.loop.call_soon_threadsafe(create_task, self.disconnect())
