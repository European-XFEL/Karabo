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
from IPython.qt import kernel_mixins, util
from IPython.kernel.inprocess.client import InProcessKernelClient
from IPython.kernel.inprocess import channels

from karabo.hash import Hash

import manager
from network import network

class IPythonWidget(RichIPythonWidget):

    """
    A convenience class for a live IPython console widget.
    """
    def __init__(self,customBanner=None, *args, **kwargs):
        if customBanner is not None:
            self.banner = customBanner
        super(IPythonWidget, self).__init__(*args,**kwargs)

        self.kernel_manager = Manager()
        self.kernel_manager.start_kernel()
        self.kernel_client = self.kernel_manager.client()
        self.kernel_client.start_channels()
        self.exit_requested.connect(self.stop)


    def stop(self):
        self.kernel_client.stop_channels()
        self.kernel_manager.shutdown_kernel()
        guisupport.get_app_qt4().exit()


    def pushVariables(self,variableDict):
        """ Given a dictionary containing name / value pairs, push those variables to the IPython console widget """
        self.kernel_manager.kernel.shell.push(variableDict)


    def clearConsole(self):
        """
        The terminal gets cleared.
        """
        self._control.clear()


    def printText(self, text):
        """
        The given \text is printed.
        """
        self._append_plain_text(text)


    def executeCommand(self, command, inFrame=False):
        """
        The given \command is executed in the frame of the console or not.
        """
        self._execute(command, inFrame)


class Channel(channels.InProcessChannel):
    def is_alive(self):
        return self.client.alive

    def receive(self, box, value, timestamp):
        if value != self.last:
            self.last = value
            self.call_handlers_later(pickle.loads(value))


class IOPubChannel(kernel_mixins.QtIOPubChannelMixin,
                   channels.InProcessIOPubChannel, Channel):
    def __init__(self, client):
        super().__init__(client)
        self.last = None
        self.client.device.boxvalue.iopub.signalUpdateComponent.connect(
            self.receive)


class ShellChannel(kernel_mixins.QtShellChannelMixin,
                   channels.InProcessShellChannel, Channel):
    def __init__(self, client):
        super().__init__(client)
        self.last = None
        self.client.device.boxvalue.shell.signalUpdateComponent.connect(
            self.receive)

    def _dispatch_to_kernel(self, msg):
        self.client.device.value.shell = pickle.dumps(msg)


class StdInChannel(kernel_mixins.QtStdInChannelMixin,
                   channels.InProcessStdInChannel, Channel):
    def __init__(self, client):
        super().__init__(client)
        self.last = None
        self.client.device.boxvalue.stdin.signalUpdateComponent.connect(
            self.receive)

    def input(self, string):
        content = dict(value=string)
        msg = self.client.session.msg('input_reply', content)
        self.client.device.value.stdin = pickle.dumps(msg)


class HBChannel(kernel_mixins.QtHBChannelMixin,
                channels.InProcessHBChannel, Channel):
    pass


class Manager(kernel_mixins.QtKernelManagerMixin):
    def start_kernel(self, **kwargs):
        self.name = "CLI-{}-{}".format(socket.gethostname(), os.getpid())
        network.onInitDevice("macroServer", "IPythonKernel", self.name, Hash())

    def client(self):
        return Client(self.name)


class Client(InProcessKernelClient, util.SuperQObject,
             metaclass=util.MetaQObjectHasTraits):
    started_channels = QtCore.pyqtSignal()
    stopped_channels = QtCore.pyqtSignal()

    shell_channel_class = ShellChannel
    iopub_channel_class = IOPubChannel
    stdin_channel_class = StdInChannel
    hb_channel_class = HBChannel

    def __init__(self, name):
        super().__init__()
        self.alive = False
        self.name = name
        self.device = manager.getDevice(self.name)
        self.device.addVisible()
        self.device.signalStatusChanged.connect(self.onStatusChanged)

    def onStatusChanged(self, device, status, error):
        self.alive = status == "alive"
        if self.alive:
            self.started_channels.emit()


    def start_channels(self):
        super(InProcessKernelClient, self).start_channels(self)
