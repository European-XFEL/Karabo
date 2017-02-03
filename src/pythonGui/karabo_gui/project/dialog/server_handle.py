#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog

from karabo.middlelayer import AccessLevel
import karabo_gui.globals as krb_globals
from karabo_gui.singletons.api import get_topology


class ServerHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(ServerHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'server_handle.ui')
        uic.loadUi(filepath, self)

        avail_hosts, avail_servers = self._get_available_hosts_servers()
        for server_id in avail_servers:
            self.cbServerId.addItem(server_id)

        for host in avail_hosts:
            self.cbHost.addItem(host)

        if model is None:
            title = 'Add server'
        else:
            title = 'Edit server'
            index = self.cbServerId.findText(model.server_id)
            if index < 0:
                server_edit = self.cbServerId.lineEdit()
                server_edit.setText(model.server_id)
            else:
                self.cbServerId.setCurrentIndex(index)

            index = self.cbHost.findText(model.host)
            if index < 0:
                host_edit = self.cbHost.lineEdit()
                host_edit.setText(model.host)
            else:
                self.cbHost.setCurrentIndex(index)
            self.teDescription.setPlainText(model.description)
        self.setWindowTitle(title)

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
        return available_hosts, available_servers

    @property
    def server_id(self):
        return self.cbServerId.currentText()

    @property
    def host(self):
        return self.cbHost.currentText()

    @property
    def author(self):
        return self.leAuthor.text()

    @property
    def copyOf(self):
        return self.leCopyOf.text()

    @property
    def description(self):
        return self.teDescription.toPlainText()
