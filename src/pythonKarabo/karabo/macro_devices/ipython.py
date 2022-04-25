import pickle
from asyncio import ensure_future, get_event_loop
from queue import Empty
from textwrap import dedent

from jupyter_client.blocking.channels import ZMQSocketChannel
from jupyter_client.channels import HBChannel
from jupyter_client.channelsabc import ChannelABC, HBChannelABC
from jupyter_client.client import KernelClient
from jupyter_client.manager import KernelManager

from karabo.common.states import State
from karabo.interactive.ikarabo import SCRIPT
from karabo.middlelayer import Device, coslot
from karabo.native import AccessLevel, AccessMode, Int32, Slot, VectorChar


class ChannelMixin(ZMQSocketChannel, ChannelABC):
    def start(self):
        self.task = ensure_future(self.loop())
        super().start()

    async def loop(self):
        loop = get_event_loop()
        while True:
            try:
                msg = await loop.run_in_executor(None, self.get_msg, True, 1)
                self.call_handlers(pickle.dumps(msg))
                value = self.device.doNotCompressEvents.value + 1
                self.device.doNotCompressEvents = value
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


class ControlChannel(ZMQSocketChannel, ChannelABC):
    pass


class HBChannelReal(HBChannel, HBChannelABC):
    pass


class Client(KernelClient):
    def __init__(self, **kwargs):
        super().__init__(shell_channel_class=ShellChannel,
                         iopub_channel_class=IOPubChannel,
                         stdin_channel_class=StdInChannel,
                         control_channel_class=ControlChannel,
                         hb_channel_class=HBChannelReal,
                         **kwargs)


class IPythonKernel(Device):
    client = None
    manager = None

    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def shell(self, msg):
        if self.client is not None:
            self.client.shell_channel.send(pickle.loads(msg))

    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def iopub(self, msg):
        if self.client is not None:
            self.client.iopub_channel.send(pickle.loads(msg))

    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def stdin(self, msg):
        if self.client is not None:
            self.client.stdin_channel.send(pickle.loads(msg))

    doNotCompressEvents = Int32(
        description=dedent("""\
            Sending this expected parameter with the change of another
            one prevents the GUI server from throtteling down fast changes"""),
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        defaultValue=0)

    visibility = Int32(enum=AccessLevel, defaultValue=AccessLevel.ADMIN)

    @Slot(requiredAccessLevel=AccessLevel.EXPERT)
    def interrupt(self):
        self.manager.interrupt_kernel()

    async def _run(self, **kwargs):
        await super()._run(**kwargs)
        self.manager = KernelManager(client_factory=Client)
        self.manager.start_kernel(
            extra_arguments=["-c", SCRIPT,
                             "--matplotlib=inline"])
        self.client = self.manager.client()
        self.client.shell_channel.device = self
        self.client.iopub_channel.device = self
        self.client.stdin_channel.device = self
        self.client.start_channels()
        self.state = State.STARTED

    def _initInfo(self):
        info = super(IPythonKernel, self)._initInfo()
        info["lang"] = "python"
        # XXX: The GUI cannot handle this device as client for the time being
        #      treat it as a macro so it will not be archived.
        info["type"] = "macro"
        return info

    @coslot
    async def slotKillDevice(self):
        self.state = State.STOPPING
        if self.manager is not None and self.manager.has_kernel:
            self.manager.request_shutdown()
            await self._ss.loop.run_in_executor(
                None, self.manager.finish_shutdown)
            self.manager = None
        self.state = State.STOPPED
        await super().slotKillDevice()
