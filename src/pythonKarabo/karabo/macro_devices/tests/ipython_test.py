import pickle
from asyncio import Future, TimeoutError, wait_for
from contextlib import contextmanager
from datetime import datetime, timezone

from flaky import flaky

from karabo.middlelayer import (
    Device, State, String, background, connectDevice, sleep, waitUntil,
    waitUntilNew)
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst

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


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = IPythonKernel({
            "_deviceId_": "test_kernel"
        })
        cls.client = ClientDevice({
            "_deviceId_": "test_kernel_device_client",
            "ipythonId": "test_kernel"
        })
        with cls.deviceManager(cls.client, lead=cls.dev):
            yield

    @async_tst
    @flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
    async def test_init(self):
        client = await connectDevice(self.client.deviceId)
        await waitUntil(lambda: client.state == State.ACTIVE)
        self.assertIsNotNone(self.client.remote)
        await waitUntilNew(client.output)
        assert client.output.value.startswith("IKarabo Version")
        ret = await self.client.execute("print('hello')\n")
        self.assertEqual(ret['status'], 'ok')
        # this test is flaky, the test client interface seems to fail
        # with ~1/20 chance.
        try:
            await wait_for(waitUntilNew(client.output), timeout=1)
        except TimeoutError:
            pass
        self.assertEqual(client.output, 'hello\n')
