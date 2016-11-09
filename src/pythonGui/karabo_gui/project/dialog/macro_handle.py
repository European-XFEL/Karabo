#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 1, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog


class MacroHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(MacroHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'macro_handle.ui')
        uic.loadUi(filepath, self)

        if model is None:
            title = 'Add macro'
        else:
            title = 'Edit macro'
            self.leTitle.setText(model.simple_name)
        self.setWindowTitle(title)
        self.buttonBox.accepted.connect(self.accept)

    @property
    def simple_name(self):
        return self.leTitle.text()
