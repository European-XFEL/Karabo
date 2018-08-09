#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog, QDialogButtonBox

from karabo.middlelayer import AccessLevel
from karabogui import globals as krb_globals
from karabogui.singletons.api import get_topology
from karabogui.util import InputValidator


class ServerHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(ServerHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'server_handle.ui')
        uic.loadUi(filepath, self)

        avail_hosts, avail_servers = self._get_available_hosts_servers()
        for server_id in avail_servers:
            self.ui_server_id.addItem(server_id)

        for host in avail_hosts:
            self.ui_host.addItem(host)

        if model is None:
            title = 'Add server'
        else:
            title = 'Edit server'
            index = self.ui_server_id.findText(model.server_id)
            if index < 0:
                server_edit = self.ui_server_id.lineEdit()
                server_edit.setText(model.server_id)
            else:
                self.ui_server_id.setCurrentIndex(index)

            index = self.ui_host.findText(model.host)
            if index < 0:
                host_edit = self.ui_host.lineEdit()
                host_edit.setText(model.host)
            else:
                self.ui_host.setCurrentIndex(index)
            self.ui_description.setPlainText(model.description)
        self.setWindowTitle(title)

        validator = InputValidator()
        self.ui_server_id.setValidator(validator)
        self.ui_host.setValidator(validator)

        self.ui_server_id.currentIndexChanged.connect(self._update_button_box)
        self.ui_server_id.editTextChanged.connect(self._update_button_box)
        self.ui_host.currentIndexChanged.connect(self._update_button_box)
        self.ui_host.editTextChanged.connect(self._update_button_box)
        self._update_button_box()

    def _get_available_hosts_servers(self):
        """ Get all available hosts and servers of the `systemTopology`
        """
        available_hosts = set()
        available_servers = set()

        def visitor(node):
            if node.attributes.get('type') != 'server':
                return

            visibility = AccessLevel(node.attributes['visibility'])
            if visibility < krb_globals.GLOBAL_ACCESS_LEVEL:
                available_servers.add(node.node_id)

            host = node.attributes.get('host', '')
            if host:
                available_hosts.add(host)

        get_topology().visit_system_tree(visitor)
        return sorted(available_hosts), sorted(available_servers)

    @pyqtSlot()
    def _update_button_box(self):
        """Only enable Ok button, if title and configuration is set
        """
        enabled = (len(self.ui_server_id.currentText()) and
                   len(self.ui_host.currentText()))
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    @property
    def server_id(self):
        return self.ui_server_id.currentText()

    @property
    def host(self):
        return self.ui_host.currentText()

    @property
    def description(self):
        return self.ui_description.toPlainText()
