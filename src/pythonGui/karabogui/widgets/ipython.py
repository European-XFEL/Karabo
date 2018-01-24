#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os
import pickle
import socket

try:
    from ipykernel.inprocess.client import InProcessKernelClient
    from qtconsole.rich_jupyter_widget import RichJupyterWidget
    from qtconsole import kernel_mixins, inprocess
except ImportError:
    from IPython.kernel.inprocess.client import InProcessKernelClient
    from IPython.qt.console.rich_ipython_widget import RichIPythonWidget \
        as RichJupyterWidget
    from IPython.qt import kernel_mixins, inprocess

from karabo.middlelayer import Hash, State
from karabogui import globals as krb_globals, messagebox
from karabogui.binding.api import get_binding_value, PropertyProxy
from karabogui.enums import KaraboSettings
from karabogui.request import send_property_changes
from karabogui.singletons.api import get_network, get_topology
from karabogui.topology.api import is_server_online
from karabogui.util import get_setting


class IPythonWidget(RichJupyterWidget):
    def __init__(self, banner=None, call_backs={}, *args, **kwargs):
        if banner is not None:
            self.banner = banner
        super().__init__(*args, local_kernel=True, **kwargs)

        self.kernel_manager = KernelManager()
        self.kernel_manager.start_kernel()
        self.kernel_client = self.kernel_manager.client()

        session_start_cb = call_backs.get('start_ch')
        session_stop_cb = call_backs.get('stop_ch')
        if session_start_cb:
            self.kernel_client.started_channels.connect(session_start_cb)
        if session_stop_cb:
            self.kernel_client.stopped_channels.connect(session_stop_cb)
        # The following line avoids a bug in IPython's QtConsole
        # see https://github.com/jupyter/qtconsole/issues/174
        self.execute_on_complete_input = False

    def stop(self):
        self.kernel_client.stop_channels()
        self.kernel_manager.shutdown_kernel()


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
        serverId = get_setting(KaraboSettings.MACRO_SERVER)
        serverId = serverId or krb_globals.MACRO_SERVER

        success = is_server_online(serverId)
        if not success:
            messagebox.show_error(
                "Macro server {} not found in system topology. "
                "Macro cannot be started.".format(serverId),
                modal=False)
            raise IOError

        hostname = socket.gethostname().replace(".", "_")
        network = get_network()
        self.name = "CLI-{}-{}".format(hostname, os.getpid())
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
        super(KernelClient, self).__init__()
        self.alive = False
        self.started = False
        self.name = name

        device = get_topology().get_device(self.name)
        self.device = device
        self.interrupt = PropertyProxy(root_proxy=device, path='interrupt')
        self._iopub = PropertyProxy(root_proxy=device, path='iopub')
        self._shell = PropertyProxy(root_proxy=device, path='shell')
        self._stdin = PropertyProxy(root_proxy=device, path='stdin')

        device.on_trait_change(self._state_update,
                               'state_binding.config_update')
        for prop in (self._iopub, self._shell, self._stdin):
            prop.on_trait_change(self._proxy_update, 'binding.config_update')
            # The device is killed when closing. No need to stop_monitoring()
            prop.start_monitoring()

    def _proxy_update(self, binding, name, event):
        # This is bound to 'binding.config_update' notifications
        if name != 'config_update':
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
        if name != 'config_update':
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
        # magic: skip InProcessKernelClient, does stuff we don't want
        super(InProcessKernelClient, self).start_channels(self)

    def input(self, string):
        msg = self.session.msg('input_reply', {'value': string})
        self._stdin.edit_value = pickle.dumps(msg)
        send_property_changes([self._stdin])
