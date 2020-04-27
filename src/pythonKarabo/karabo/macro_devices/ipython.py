from asyncio import coroutine, ensure_future, get_event_loop
from queue import Empty
import pickle
from textwrap import dedent

from jupyter_client.blocking.channels import ZMQSocketChannel
from jupyter_client.channels import HBChannel
from jupyter_client.channelsabc import ChannelABC, HBChannelABC
from jupyter_client.client import KernelClient
from jupyter_client.manager import KernelManager

from karabo.common.states import State
from karabo.interactive.ikarabo import SCRIPT
from karabo.native.data.enums import AccessLevel, AccessMode, Assignment
from karabo.native.data.hash import Bool, Int32, Slot, VectorChar

from karabo.middlelayer import coslot, Device


class ChannelMixin(ZMQSocketChannel, ChannelABC):
    def start(self):
        self.task = ensure_future(self.loop())
        super().start()

    @coroutine
    def loop(self):
        loop = get_event_loop()
        while True:
            try:
                msg = yield from loop.run_in_executor(None, self.get_msg,
                                                      True, 1)
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
    client = None
    manager = None

    archive = Bool(
        displayedName="Archive", accessMode=AccessMode.RECONFIGURABLE,
        assignment=Assignment.OPTIONAL, defaultValue=False)

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

    @coroutine
    def _run(self, **kwargs):
        yield from super()._run(**kwargs)
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
        info["type"] = "device"
        return info

    @coslot
    def slotKillDevice(self):
        self.state = State.STOPPING
        if self.manager is not None and self.manager.has_kernel:
            self.manager.request_shutdown()
            yield from self._ss.loop.run_in_executor(
                None, self.manager.finish_shutdown)
            self.manager = None
        self.state = State.STOPPED
        yield from super().slotKillDevice()
