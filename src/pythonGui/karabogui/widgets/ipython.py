#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2011
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
import os
import pickle
import socket

from ipykernel.inprocess.client import InProcessKernelClient
from qtconsole import inprocess, kernel_mixins
from qtconsole.rich_jupyter_widget import RichJupyterWidget

from karabo.common.states import State
from karabo.native import Hash
from karabogui import messagebox
from karabogui.binding.api import PropertyProxy, get_binding_value
from karabogui.request import send_property_changes
from karabogui.singletons.api import get_network, get_topology
from karabogui.topology.api import get_macro_servers


class IPythonWidget(RichJupyterWidget):
    def __init__(self, banner=None, call_backs={}, *args, **kwargs):
        if banner is not None:
            self.banner = banner
        super().__init__(*args, local_kernel=True, **kwargs)

        self.kernel_manager = KernelManager()
        self.kernel_manager.start_kernel()
        self.kernel_client = self.kernel_manager.client()

        session_start_cb = call_backs.get("start_ch")
        session_stop_cb = call_backs.get("stop_ch")
        if session_start_cb:
            self.kernel_client.started_channels.connect(session_start_cb)
        if session_stop_cb:
            self.kernel_client.stopped_channels.connect(session_stop_cb)

    def stop(self):
        self.kernel_client.stop_channels()
        self.kernel_manager.shutdown_kernel()

    def destroy(self):
        # Cleanup first
        self.kernel_client.started_channels.disconnect()
        self.kernel_client.stopped_channels.disconnect()
        self.deleteLater()
        super().destroy()


class Channel(inprocess.QtInProcessChannel):
    def __init__(self, *args):
        super().__init__(*args)
        self.last = None

    def is_alive(self):
        return self.client.alive

    def receive(self, value):
        if value != self.last:
            self.last = value
            self.call_handlers_later(pickle.loads(value))


class KernelManager(kernel_mixins.QtKernelManagerMixin):
    __client = None

    def start_kernel(self):
        macro_servers = get_macro_servers()
        if not macro_servers:
            messagebox.show_error(
                "No Macro server found in system topology. "
                "Console cannot be started.")
            raise OSError

        hostname = socket.gethostname().replace(".", "_")
        network = get_network()
        self.name = f"CLI-{hostname}-{os.getpid()}"
        serverId = macro_servers[0]
        network.onInitDevice(serverId, "IPythonKernel", self.name, Hash())

    def shutdown_kernel(self):
        network = get_network()
        network.onKillDevice(self.name)

    def client(self):
        if self.__client is None:
            self.__client = KernelClient(self.name)
        return self.__client

    def interrupt_kernel(self):
        self.__client.interrupt.execute()


class KernelClient(inprocess.QtInProcessKernelClient):
    shell_channel_class = Channel
    iopub_channel_class = Channel
    stdin_channel_class = Channel
    hb_channel_class = inprocess.QtInProcessHBChannel

    def __init__(self, name):
        super().__init__()
        self.alive = False
        self.started = False
        self.name = name

        device = get_topology().get_device(self.name)
        self.device = device
        self.interrupt = PropertyProxy(root_proxy=device, path="interrupt")
        self._iopub = PropertyProxy(root_proxy=device, path="iopub")
        self._shell = PropertyProxy(root_proxy=device, path="shell")
        self._stdin = PropertyProxy(root_proxy=device, path="stdin")

        device.on_trait_change(self._state_update,
                               "state_binding.config_update")
        for prop in (self._iopub, self._shell, self._stdin):
            prop.on_trait_change(self._proxy_update, "binding.config_update")
            # The device is killed when closing. No need to stop_monitoring()
            prop.start_monitoring()

    def _proxy_update(self, binding, name, event):
        # This is bound to "binding.config_update" notifications
        if name != "config_update":
            return
        mapping = {
            self._iopub: self.iopub_channel,
            self._shell: self.shell_channel,
            self._stdin: self.stdin_channel,
        }
        for proxy, channel in mapping.items():
            if proxy.binding is binding:
                value = get_binding_value(proxy)
                if value is not None:
                    channel.receive(value)
                break

    def _state_update(self, binding, name, event):
        if name != "config_update":
            return

        value = get_binding_value(binding, State.UNKNOWN)
        self.alive = State(value) is State.STARTED
        if self.alive:
            if not self.started:
                self.start_channels()
                self.started = True
                self.started_channels.emit()

    def _dispatch_to_kernel(self, msg):
        self._shell.edit_value = pickle.dumps(msg)
        send_property_changes([self._shell])

    def start_channels(self):
        # magic: skip InProcessKernelClient, does stuff we don"t want
        super(InProcessKernelClient, self).start_channels()

    def input(self, string):
        msg = self.session.msg("input_reply", {"value": string})
        self._stdin.edit_value = pickle.dumps(msg)
        send_property_changes([self._stdin])
