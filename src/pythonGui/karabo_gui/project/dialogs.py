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

    def set_dialog_texts(self, title, btn_text):
        """ This method sets the ``title`` and the ``btn_text`` of the ok
        button.

        :param title: The new window title
        :param btn_text: The new text for ok button of the ``QDialogButtonBox``
        """
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btn_text)


class NewDialog(ProjectHandleDialog):
    def __init__(self, title="New Project", btn_text="New", parent=None):
        super(NewDialog, self).__init__(parent)

        self.set_dialog_texts(title, btn_text)
        self.buttonBox.accepted.connect(self.new_clicked)

    @pyqtSlot()
    def new_clicked(self):
        pass


class LoadDialog(ProjectHandleDialog):
    def __init__(self, title="Load Project", btn_text="Load", parent=None):
        super(LoadDialog, self).__init__(parent)

        self.set_dialog_texts(title, btn_text)
        self.buttonBox.accepted.connect(self.load_clicked)

    @pyqtSlot()
    def load_clicked(self):
        pass


class SaveDialog(ProjectHandleDialog):
    def __init__(self, title="Save Project", btn_text="Save", parent=None):
        super(SaveDialog, self).__init__(parent)

        self.set_dialog_texts(title, btn_text)
        self.buttonBox.accepted.connect(self.save_clicked)

    @pyqtSlot()
    def save_clicked(self):
        pass
