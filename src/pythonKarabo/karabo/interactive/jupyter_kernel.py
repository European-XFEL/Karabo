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
import os
import socket
import sys
from asyncio import get_event_loop, set_event_loop

from ipykernel.ipkernel import IPythonKernel
from ipykernel.jsonutil import json_clean
from ipykernel.kernelapp import IPKernelApp
from tornado.platform.asyncio import AsyncIOMainLoop
from zmq.asyncio import ZMQEventLoop

from karabo.middlelayer import Device, DeviceClientBase, background, eventloop


class EventLoop(eventloop.EventLoop, ZMQEventLoop):
    pass


class KaraboKernel(IPythonKernel):
    def execute_request(self, stream, ident, parent):
        """ handle an execute_request

        This is effectively a copy from ipykernel.kernelbase.Kernel.
        The original version however blocks the event loop, which we cannot
        do as Karabo functions still need to be connected to the broker.

        So we offload every command into a thread.
        """
        try:
            content = parent['content']
            code = content['code']
            silent = content['silent']
            store_history = content.get('store_history', not silent)
            user_expressions = content.get('user_expressions', {})
            allow_stdin = content.get('allow_stdin', False)
        except Exception as e:
            self.device.logger.exception(f"Got bad msg: {parent} - {e}")
            return

        stop_on_error = content.get('stop_on_error', True)

        metadata = self.init_metadata(parent)

        # Re-broadcast our input for the benefit of listening clients, and
        # start computing output
        if not silent:
            self.execution_count += 1
            self._publish_execute_input(code, parent, self.execution_count)

        async def execute():
            reply_future = await background(
                self.do_execute, code, silent, store_history, user_expressions,
                allow_stdin)

            # Flush output before sending the reply.
            sys.stdout.flush()
            sys.stderr.flush()

            # Send the reply.
            reply_content = json_clean(reply_future.result())
            finished = self.finish_metadata(parent, metadata, reply_content)

            reply_msg = self.session.send(
                stream, 'execute_reply', reply_content, parent,
                metadata=finished, ident=ident)

            if (not silent and reply_msg['content']['status'] == 'error'
                    and stop_on_error):
                self._abort_queues()
        get_event_loop().create_task(execute(), self.device)


class JupyterDevice(DeviceClientBase, Device):
    def __init__(self, configuration):
        super().__init__(configuration)

    def _initInfo(self):
        info = super()._initInfo()
        info["lang"] = "python"
        info["type"] = "client"
        return info


class KaraboKernelApp(IPKernelApp):
    name = "karabo-kernel"
    kernel_class = KaraboKernel
    exec_lines = ["%pylab inline",
                  "from karabo.middlelayer import *",
                  "from karabo.middlelayer.numeric import *"]

    def start(self):
        hostname = socket.gethostname().split(".", 1)[0]
        pid = os.getpid()
        loop = get_event_loop()
        self.kernel.device = JupyterDevice(
            {"_deviceId_": f"jupyter-{hostname}-{pid}"})
        loop.run_until_complete(
            self.kernel.device.startInstance())
        self.kernel.start()
        loop.run_forever()


if __name__ == "__main__":
    loop = EventLoop()
    set_event_loop(loop)
    AsyncIOMainLoop().install()
    app = KaraboKernelApp.instance()
    app.initialize()
    app.start()
