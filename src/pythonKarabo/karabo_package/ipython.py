import pickle

from IPython.kernel.manager import KernelManager
from IPython.kernel import KernelClient
from IPython.kernel import channels

from karabo.python_device import Device
from karabo.hashtypes import VectorChar
from karabo.signalslot import coslot


class ChannelMixin(object):
    def call_handlers(self, msg):
        self.device._ss.loop.call_soon_threadsafe(
            self.call_handlers_later, msg)


class ShellChannel(ChannelMixin, channels.ShellChannel):
    def call_handlers_later(self, msg):
        self.device.shell = pickle.dumps(msg)


class IOPubChannel(ChannelMixin, channels.IOPubChannel):
    def call_handlers_later(self, msg):
        self.device.iopub = pickle.dumps(msg)


class StdInChannel(ChannelMixin, channels.StdInChannel):
    def call_handlers_later(self, msg):
        self.device.stdin = pickle.dumps(msg)


class Client(KernelClient):
    def __init__(self, **kwargs):
        super().__init__(shell_channel_class=ShellChannel,
                         iopub_channel_class=IOPubChannel,
                         stdin_channel_class=StdInChannel, **kwargs)


class IPythonKernel(Device):
    @VectorChar()
    def shell(self, msg):
        self.client.shell_channel._queue_send(pickle.loads(msg))

    @VectorChar()
    def iopub(self, msg):
        self.client.iopub_channel._queue_send(pickle.loads(msg))

    @VectorChar()
    def stdin(self, msg):
        self.client.stdin_channel._queue_send(pickle.loads(msg))

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
