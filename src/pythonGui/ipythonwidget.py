#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["IPythonWidget"]

import os
import pickle
import socket

from PyQt4 import QtCore

from IPython.qt.console.rich_ipython_widget import RichIPythonWidget
from IPython.lib import guisupport
from IPython.qt import kernel_mixins, inprocess, util
from IPython.kernel.inprocess.client import InProcessKernelClient
from IPython.kernel.inprocess import channels

from karabo.hash import Hash

import manager
from network import network

class IPythonWidget(RichIPythonWidget):
    def __init__(self, banner=None, *args, **kwargs):
        if banner is not None:
            self.banner = banner
        super().__init__(*args, local_kernel=True, **kwargs)

        self.kernel_manager = Manager()
        self.kernel_manager.start_kernel()
        self.kernel_client = self.kernel_manager.client()
        self.exit_requested.connect(self.stop)

    def stop(self):
        self.kernel_client.stop_channels()
        self.kernel_manager.shutdown_kernel()


class Channel(inprocess.QtInProcessChannel):
    def __init__(self, *args):
        super().__init__(*args)
        self.last = None

    def is_alive(self):
        return self.client.alive

    def receive(self, box, value, timestamp):
        if value != self.last:
            self.last = value
            self.call_handlers_later(pickle.loads(value))


class Manager(kernel_mixins.QtKernelManagerMixin):
    def start_kernel(self):
        self.name = "CLI-{}-{}".format(socket.gethostname(), os.getpid())
        network.onInitDevice("macroServer", "IPythonKernel", self.name, Hash())

    def shutdown_kernel(self):
        network.onKillDevice(self.name)

    def client(self):
        return Client(self.name)


class Client(inprocess.QtInProcessKernelClient):
    shell_channel_class = Channel
    iopub_channel_class = Channel
    stdin_channel_class = Channel
    hb_channel_class = inprocess.QtInProcessHBChannel

    def __init__(self, name):
        super().__init__()
        self.alive = False
        self.started = False
        self.name = name
        self.device = manager.getDevice(self.name)
        self.device.addVisible()
        self.device.signalStatusChanged.connect(self.onStatusChanged)
        self.device.boxvalue.iopub.signalUpdateComponent.connect(
            self.iopub_channel.receive)
        self.device.boxvalue.shell.signalUpdateComponent.connect(
            self.shell_channel.receive)
        self.device.boxvalue.stdin.signalUpdateComponent.connect(
            self.stdin_channel.receive)

    def onStatusChanged(self, device, status, error):
        self.alive = status == "alive"
        if self.alive:
            if not self.started:
                self.start_channels()
                self.started = True
                self.started_channels.emit()

    def _dispatch_to_kernel(self, msg):
        self.device.value.shell = pickle.dumps(msg)

    def start_channels(self):
        # magic: skip InProcessKernelClient, does stuff we don't want
        super(InProcessKernelClient, self).start_channels(self)

    def input(self, string):
        content = dict(value=string)
        msg = self.session.msg('input_reply', content)
        self.device.value.stdin = pickle.dumps(msg)
