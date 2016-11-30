#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog


class ObjectSaveDialog(QDialog):
    def __init__(self, model, parent=None):
        """ The dialog expects a ``model``
        """
        super(ObjectSaveDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'object_save.ui')
        uic.loadUi(filepath, self)
        self.setWindowTitle('Save object {}'.format(model.simple_name))
        self.leAlias.setText(model.alias)

    @property
    def alias(self):
        return self.leAlias.text()
