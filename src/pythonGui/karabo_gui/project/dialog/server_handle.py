#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog

from karabo.middlelayer import Hash
from karabo_gui.singletons.api import get_manager


class ServerHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(ServerHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'server_handle.ui')
        uic.loadUi(filepath, self)

        for host in self._get_available_hosts():
            self.cbHost.addItem(host)

        if model is None:
            title = 'Add server'
        else:
            title = 'Edit server'
            self.leServerId.setText(model.server_id)
            index = self.cbHost.findText(model.host)
            # NOTE: index might be -1 if model.host is not online (or empty)
            # QComboBox handles this correctly, but the user should be notified
            self.cbHost.setCurrentIndex(index)
            self.teDescription.setPlainText(model.description)
        self.setWindowTitle(title)

    def _get_available_hosts(self):
        """ Get all available hosts of `systemTopology`
        """
        available_hosts = set()
        servers = get_manager().systemHash.get('server', Hash())
        for server_id, _, attrs in servers.iterall():
            if not attrs:
                continue

            host = attrs.get("host", "")
            if host:
                available_hosts.add(host)
        return available_hosts

    @property
    def server_id(self):
        return self.leServerId.text()

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
