import pickle
from asyncio import ensure_future
from textwrap import dedent

from jupyter_client import AsyncKernelClient, AsyncKernelManager
from jupyter_client.channels import ZMQSocketChannel
from traitlets import Type

from karabo.common.states import State
from karabo.middlelayer import Device
from karabo.native import AccessLevel, AccessMode, Int32, Slot, VectorChar


class ChannelMixin(ZMQSocketChannel):
    device = None
    task = None

    # Overload ZMQSocketChannel functions
    async def get_msg(self, timeout=None):
        msg = await super().get_msg(timeout)
        self.set_karabo_value(pickle.dumps(msg))
        value = self.device.doNotCompressEvents.value + 1
        self.device.doNotCompressEvents = value
        self.device.update()
        return msg

    def start(self):
        """Implement the start function

        start a listening task"""
        self.task = ensure_future(self._listen())
        super().start()

    def stop(self):
        """Implement the stop function

        stop the listening task"""
        if self.task is not None:
            self.task.cancel()
            self.task = None
        super().stop()

    # The listening loop
    async def _listen(self):
        while True:
            await self.get_msg()


class ShellChannel(ChannelMixin):
    def set_karabo_value(self, msg):
        self.device.shell = msg


class IOPubChannel(ChannelMixin):
    def set_karabo_value(self, msg):
        self.device.iopub = msg


class StdInChannel(ChannelMixin):
    def set_karabo_value(self, msg):
        self.device.stdin = msg


class ControlChannel(ChannelMixin):
    def set_karabo_value(self, msg):
        self.device.control = msg


class Client(AsyncKernelClient):
    shell_channel_class = Type(ShellChannel)
    iopub_channel_class = Type(IOPubChannel)
    stdin_channel_class = Type(StdInChannel)
    control_channel_class = Type(ControlChannel)


class KernelManager(AsyncKernelManager):
    client_factory = Client


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

    @VectorChar(
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.EXPERT)
    def control(self, msg):
        if self.client is not None:
            self.client.control_channel.send(pickle.loads(msg))

    doNotCompressEvents = Int32(
        description=dedent("""\
            Sending this expected parameter with the change of another
            one prevents the GUI server from throtteling down fast changes"""),
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        defaultValue=0)

    visibility = Int32(enum=AccessLevel, defaultValue=AccessLevel.ADMIN)

    @Slot(requiredAccessLevel=AccessLevel.EXPERT)
    async def interrupt(self):
        await self.kill_kernel()

    async def _run(self, **kwargs):
        await super()._run(**kwargs)
        try:
            self.manager = KernelManager(kernel_name="karabo")
            await self.manager.start_kernel()
            await self.manager.is_alive()
            client = self.manager.client()
            client.shell_channel.device = self
            client.iopub_channel.device = self
            client.stdin_channel.device = self
            client.control_channel.device = self
            client.start_channels()
            self.client = client
            self.state = State.STARTED
        except RuntimeError as e:
            await self.kill_kernel()
            self.logger.exception("error on initialization")
            raise e

    def _initInfo(self):
        info = super(IPythonKernel, self)._initInfo()
        info["lang"] = "python"
        # A macro so it will not be archived
        info["type"] = "macro"
        return info

    async def kill_kernel(self):
        if self.manager is not None and self.manager.has_kernel:
            self.client.stop_channels()
            self.client = None
            # tries to shutdown nicely and forces shutdown after 5 seconds
            await self.manager.shutdown_kernel()
            self.manager = None

    async def onDestruction(self):
        self.state = State.STOPPING
        await self.kill_kernel()
        self.state = State.STOPPED
