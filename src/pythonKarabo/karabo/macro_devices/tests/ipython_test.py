from asyncio import Future
from contextlib import contextmanager
from functools import partial

from karabo.middlelayer import State, connectDevice, waitUntil
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst

from ..ipython import IPythonKernel


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = IPythonKernel({
            "_deviceId_": "test_kernel"
        })
        with cls.deviceManager(lead=cls.dev):
            yield

    @async_tst
    async def test_kernel_client(self):
        dev = await connectDevice(self.dev.deviceId)
        await waitUntil(lambda: dev.state == State.STARTED)
        # cancel the listening tasks
        self.dev.client.iopub_channel.task.cancel()
        self.dev.client.shell_channel.task.cancel()
        self.dev.client.stdin_channel.task.cancel()
        self.dev.client.control_channel.task.cancel()

        def wait_for_result(expected, f, msg):
            if msg["header"]["msg_type"] == "stream":
                self.assertEqual(msg['content']['name'], "stdout")
                self.assertEqual(msg['content']['text'], expected)
                f.set_result(None)

        execute = self.dev.client._async_execute_interactive

        # check the basic functionality of python
        f = Future()
        r = await execute(
            code="print('hello')",
            output_hook=partial(wait_for_result, "hello\n", f))
        self.assertEqual(r['content']['status'], 'ok')
        await f

        # check that karabo is imported and does not causes errors
        f = Future()
        r = await execute(
            code="print(type(getDevices()))",
            output_hook=partial(wait_for_result, "<class 'list'>\n", f))
        self.assertEqual(r['content']['status'], 'ok')
        await f
