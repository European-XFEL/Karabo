"""This tests the functionality of the IPythonKernel device"""
import pickle
from contextlib import contextmanager

from ipykernel.inprocess.channels import InProcessChannel
from ipykernel.inprocess.client import InProcessKernelClient

from karabo.middlelayer import (
    Device, String, background, connectDevice, waitUntilNew)
from karabo.middlelayer_api.ipython import IPythonKernel
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst

TEST_REMOTE = "CLI_REMOTE_IPYTHON_KERNEL"


class Channel(InProcessChannel):
    def __init__(self, *args):
        super().__init__(*args)
        self.last = None
        self.hist = []
        self.name = ""

    def receive(self, value):
        if value != self.last:
            self.last = value
            dec = pickle.loads(value)
            self.hist.append(dec)


class IPClient(InProcessKernelClient):
    shell_channel_class = Channel
    iopub_channel_class = Channel
    stdin_channel_class = Channel
    hb_channel_class = InProcessChannel

    def __init__(self, name):
        super().__init__()
        self.alive = False
        self.started = False
        self.name = name
        self.shell_input = ""

    def _dispatch_to_kernel(self, msg):
        """implement _dispatch_to_kernel for the parent"""
        self.device.shell = pickle.dumps(msg)

    async def start(self):
        device = await connectDevice(self.name)
        self.device = device
        super(InProcessKernelClient, self).start_channels(self)
        background(self.monitor)

    async def monitor(self):
        mapping = {
            "iopub": self.iopub_channel,
            "shell": self.shell_channel,
            "stdin": self.stdin_channel,
        }
        for prop, channel in mapping.items():
            channel.name = prop
        while True:
            # one cannot unfortunately wait on the channels
            # self.device.iopub, etc. therefore we use this.
            await waitUntilNew(self.device)
            for prop, channel in mapping.items():
                d = getattr(self.device, prop)
                value = d.value
                if value is not None:
                    channel.receive(value)


class ConsoleUser(Device):
    remoteDeviceId = String()

    def __init__(self, configuration, *args):
        super().__init__(configuration, *args)
        self.client = IPClient(self.remoteDeviceId.value)

    async def onInitialization(self):
        await self.client.start()


class TestRemoteConsole(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.device = IPythonKernel({"_deviceId_": TEST_REMOTE})
        cls.cli = ConsoleUser({"_deviceId_": "CLI_CLIENT",
                               "remoteDeviceId": TEST_REMOTE})
        with cls.deviceManager(cls.cli, lead=cls.device):
            yield

    @async_tst(timeout=10)
    async def test_remote_console(self):
        device = await connectDevice(TEST_REMOTE)
        # wait for the remote to be ready.
        with device:
            wait = True
            while wait:
                await waitUntilNew(device.iopub)
                for i in self.cli.client.iopub_channel.hist:
                    if i['msg_type'] == 'status':
                        if i['content'] == {'execution_state': 'idle'}:
                            wait = False
                            break
        # clean up the message history
        self.cli.client.iopub_channel.hist = []
        printed_string = "GOOD"
        # send a simple python command
        r = self.cli.client.execute(f'print("{printed_string}")')
        with device:
            wait = True
            while wait:
                await waitUntilNew(device.iopub)
                for i in self.cli.client.iopub_channel.hist:
                    # we have a reply!
                    if i['msg_type'] == 'stream' and\
                            i['parent_header']['msg_id'] == r:
                        wait = False
                        # let us check if it did what we wanted
                        incoming = i['content']['text'].strip()
                        self.assertEqual(printed_string, incoming)
                        break
        # critically we should not timeout
