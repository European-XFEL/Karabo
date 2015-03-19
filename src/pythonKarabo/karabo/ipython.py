from asyncio import async, coroutine, get_event_loop
from queue import Empty
import pickle

from IPython.kernel.blocking.channels import ZMQSocketChannel
from IPython.kernel.channels import HBChannel
from IPython.kernel.channelsabc import ChannelABC, HBChannelABC
from IPython.kernel.manager import KernelManager
from IPython.kernel import KernelClient

from karabo.enums import AccessLevel, AccessMode
from karabo.python_device import Device
from karabo.hashtypes import VectorChar
from karabo.signalslot import coslot


class ChannelMixin(ZMQSocketChannel, ChannelABC):
    def start(self):
        self.task = async(self.loop())
        super().start()

    @coroutine
    def loop(self):
        loop = get_event_loop()
        while True:
            try:
                msg = yield from loop.run_in_executor(None, self.get_msg,
                                                      True, 1)
                self.call_handlers(pickle.dumps(msg))
                self.device.update()
            except Empty:
                pass


class ShellChannel(ChannelMixin):
    def call_handlers(self, msg):
        self.device.shell = msg


class IOPubChannel(ChannelMixin):
    def call_handlers(self, msg):
        self.device.iopub = msg


class StdInChannel(ChannelMixin):
    def call_handlers(self, msg):
        self.device.stdin = msg


class HBChannelReal(HBChannel, HBChannelABC):
    pass


class Client(KernelClient):
    def __init__(self, **kwargs):
        super().__init__(shell_channel_class=ShellChannel,
                         iopub_channel_class=IOPubChannel,
                         stdin_channel_class=StdInChannel,
                         hb_channel_class=HBChannelReal,
                         **kwargs)


class IPythonKernel(Device):
    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def shell(self, msg):
        self.client.shell_channel.send(pickle.loads(msg))

    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def iopub(self, msg):
        self.client.iopub_channel.send(pickle.loads(msg))

    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def stdin(self, msg):
        self.client.stdin_channel.send(pickle.loads(msg))

    def run(self):
        super().run()
        self.manager = KernelManager(client_factory=Client)
        self.manager.start_kernel()
        self.client = self.manager.client()
        self.client.shell_channel.device = self
        self.client.iopub_channel.device = self
        self.client.stdin_channel.device = self
        self.client.start_channels()

    @coslot
    def slotKillDevice(self):
        yield from self._ss.loop.run_in_executor(None,
                                                 self.manager.shutdown_kernel)
        yield from super().slotKillDevice()
