#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog


class DeviceHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(DeviceHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_handle.ui')
        uic.loadUi(filepath, self)

        if model is None:
            title = 'Add device'
        else:
            title = 'Edit device'
            self.leTitle.setText(model.instance_id)
        self.setWindowTitle(title)
        self.buttonBox.accepted.connect(self.accept)

    @property
    def instance_id(self):
        return self.leTitle.text()
