import os
import socket
from asyncio import get_event_loop, set_event_loop

from ipykernel.ipkernel import IPythonKernel
from ipykernel.kernelapp import IPKernelApp
from tornado.platform.asyncio import AsyncIOMainLoop
from zmq.asyncio import ZMQEventLoop

from karabo._version import version
from karabo.middlelayer import Device, DeviceClientBase
from karabo.middlelayer_api import eventloop


class EventLoop(eventloop.EventLoop, ZMQEventLoop):
    pass


class KaraboKernel(IPythonKernel):
    banner = f"IKarabo Version {version}\n        Type karabo? for help\n"

    async def do_execute(self, code, silent, store_history=True,
                         user_expressions=None, allow_stdin=False):
        """Runs the IPythonKernel do_execute as a karabo device

        passing self.device as the instance in the loop"""
        coro = super().do_execute(
            code, silent, store_history, user_expressions, allow_stdin)
        return await get_event_loop().create_task(coro, self.device)

    def do_shutdown(self, restart):
        ret = super().do_shutdown(restart)

        async def die():
            await self.device.slotKillDevice()

        get_event_loop().create_task(die())
        return ret


class JupyterDevice(DeviceClientBase, Device):
    def __init__(self, configuration):
        super(JupyterDevice, self).__init__(configuration)

    def _initInfo(self):
        info = super(JupyterDevice, self)._initInfo()
        info["lang"] = "python"
        info["type"] = "client"
        return info


class KaraboKernelApp(IPKernelApp):
    name = "karabo-kernel"
    kernel_class = KaraboKernel
    exec_lines = ["%matplotlib inline",
                  "from karabo.middlelayer import *",
                  "from karabo.middlelayer_api.numeric import *",
                  "import karabo"]

    def start(self):
        hostname = socket.gethostname().split(".", 1)[0]
        pid = os.getpid()
        loop = get_event_loop()
        self.kernel.device = JupyterDevice(
            {"_deviceId_": "jupyter-{}-{}".format(hostname, pid)})
        loop.run_until_complete(
            self.kernel.device.startInstance())
        self.kernel.start()
        loop.run_forever()

    def close(self):
        get_event_loop().close()
        super().close()


if __name__ == "__main__":
    loop = EventLoop()
    set_event_loop(loop)
    AsyncIOMainLoop().install()
    app = KaraboKernelApp.instance()
    app.initialize()
    app.start()
