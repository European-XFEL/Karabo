#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog


class ServerHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(ServerHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'server_handle.ui')
        uic.loadUi(filepath, self)

        if model is None:
            title = 'Add server'
        else:
            title = 'Edit server'
            self.leServerId.setText(model.server_id)
        self.setWindowTitle(title)
        self.buttonBox.accepted.connect(self.accept)

    def server_id(self):
        return self.leServerId.text()

    def host(self):
        return self.leHost.text()

    def author(self):
        return self.leAuthor.text()

    def copyOf(self):
        return self.leCopyOf.text()

    def description(self):
        return self.leDescription.text()
