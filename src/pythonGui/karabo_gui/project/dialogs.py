#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog, QDialogButtonBox


class ProjectHandleDialog(QDialog):
    def __init__(self, parent=None):
        super(ProjectHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'project_handle.ui')
        uic.loadUi(filepath, self)


class NewDialog(ProjectHandleDialog):
    def __init__(self, title="New Project", btnText="New", parent=None):
        super(NewDialog, self).__init__(parent)

        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btnText)
        self.buttonBox.accepted.connect(self.new_clicked)

    @pyqtSlot()
    def new_clicked(self):
        pass


class LoadDialog(ProjectHandleDialog):
    def __init__(self, title="Load Project", btnText="Load", parent=None):
        super(LoadDialog, self).__init__(parent)

        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btnText)
        self.buttonBox.accepted.connect(self.load_clicked)

    @pyqtSlot()
    def load_clicked(self):
        pass


class SaveDialog(ProjectHandleDialog):
    def __init__(self, title="Save Project", btnText="Save", parent=None):
        super(SaveDialog, self).__init__(parent)
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btnText)
        self.buttonBox.accepted.connect(self.save_clicked)

    @pyqtSlot()
    def save_clicked(self):
        pass
