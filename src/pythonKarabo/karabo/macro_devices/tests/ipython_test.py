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
import pickle
from asyncio import Future, TimeoutError, wait_for
from datetime import datetime, timezone

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Device, State, String, background, connectDevice, sleep, waitUntil,
    waitUntilNew)
from karabo.middlelayer.testing import AsyncDeviceContext, event_loop

from ..ipython import IPythonKernel

FLAKY_MAX_RUNS = 2
FLAKY_MIN_PASSES = 1


class ClientDevice(Device):
    """This is a very crude replication of a jupyter client

    TODO: remove the custom client code and rather extend a jupyter client
    """
    ipythonId = String()
    output = String()

    # the proxy to the IPythonKernel device
    remote = None
    msgs = None

    _header = None
    _selector = None

    async def monitor(self):
        while True:
            await waitUntilNew(self.remote.doNotCompressEvents)
            self.process()

    def process(self):
        for key in self.msgs.keys():
            channel = getattr(self.remote, key)
            if channel.value is None:
                continue
            msg = pickle.loads(channel.value)
            if msg == self.msgs[key]:
                continue
            # print(key, msg)
            self._header = msg['header']
            if self._selector:
                self._selector(key, msg)

            if (key == 'iopub'
                    and msg['msg_type'] == 'stream'
                    and msg['content']['name'] == 'stdout'):
                self.output = msg['content']['text']

    async def onInitialization(self):
        self.remote = await connectDevice(self.ipythonId)
        self.msgs = dict()
        self.msgs["shell"] = None
        self.msgs["stdin"] = None
        self.msgs["iopub"] = None
        background(self.monitor())
        self.state = State.ACTIVE

    def header(self, msg_type):
        self._header['msg_type'] = msg_type
        count = self._header['msg_id'].split('_')
        if len(count) == 1:
            count = 1
        else:
            count = int(count[-1]) + 1
        self._header['msg_id'] = '_'.join((self._header['session'],
                                           str(count)))
        self._header['date'] = datetime.utcnow().replace(tzinfo=timezone.utc)
        return self._header

    async def execute(self, code):
        while self._header is None:
            await sleep(0.01)
        header = self.header('execute_request')
        msg = dict()
        msg['header'] = header
        this_id = header['msg_id']
        msg['msg_id'] = this_id
        msg['msg_type'] = header['msg_type']
        msg['content'] = {
            'code': code, 'silent': True, 'store_history': True,
            'user_expressions': {}, 'allow_stdin': True}
        msg['metadata'] = {}
        msg['parent_header'] = {}
        self.remote.shell = pickle.dumps(msg)
        future = Future()

        def select(channel, msg_):
            nonlocal this_id, future
            if channel != 'shell':
                return
            if msg_['msg_type'] != 'execute_reply':
                return
            if msg_['parent_header']['msg_id'] != this_id:
                return
            future.set_result(msg_['content'])

        self._selector = select
        ret = await future
        self._selector = None
        return ret


@pytest_asyncio.fixture()
async def deviceTest(event_loop: event_loop):
    dev = IPythonKernel({
        "_deviceId_": "test_kernel"
    })
    client = ClientDevice({
        "_deviceId_": "test_kernel_device_client",
        "ipythonId": "test_kernel"
    })
    async with AsyncDeviceContext(client=client, dev=dev) as ctx:
        yield ctx


@pytest.mark.flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_init_macros(deviceTest):
    client_dev = deviceTest["client"]
    client = await connectDevice(client_dev.deviceId)
    await waitUntil(lambda: client.state == State.ACTIVE)
    assert client_dev.remote is not None
    await waitUntilNew(client.output)
    assert client.output.value.startswith("IKarabo Version")
    ret = await client_dev.execute("print('hello')\n")
    assert ret['status'] == 'ok'
    # this test is flaky, the test client interface seems to fail
    # with ~1/20 chance.
    try:
        await wait_for(waitUntilNew(client.output), timeout=1)
    except TimeoutError:
        pass
    assert client.output == 'hello\n'
