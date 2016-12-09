#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 29, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog

from karabo_gui.util import SignalBlocker


class ObjectSaveDialog(QDialog):
    def __init__(self, alias=None, parent=None):
        """ The dialog expects a ``alias`` string

        :param alias: A string which could be used for saving a project object
        :param parent: A parent object
        """
        super(ObjectSaveDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'object_save.ui')
        uic.loadUi(filepath, self)
        self.setWindowTitle('Set version alias')
        if alias is not None:
            self.leAlias.setText(alias)

    @property
    def alias(self):
        return self.leAlias.text()


class ObjectDuplicateDialog(QDialog):
    def __init__(self, simple_name, parent=None):
        """ The dialog expects a ``simple_name`` string

        :param simple_name: A string which could be used for creating the new
                            duplicate simple Name
        :param parent: A parent object
        """
        super(ObjectDuplicateDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'object_duplicate.ui')
        uic.loadUi(filepath, self)
        self.setWindowTitle('Duplicate object {}'.format(simple_name))

        self.leTitle.setText(simple_name)
        self._update_text()
        self.sbStart.valueChanged.connect(self._indexChanged)
        self.sbEnd.valueChanged.connect(self._indexChanged)

    def _update_text(self):
        start_index = self.sbStart.value()
        end_index = self.sbEnd.value()
        # Update text
        nb_dupe = end_index - start_index + 1
        text = 'You are about to create <b>{}</b> duplicate(s)'.format(nb_dupe)
        self.laText.setText(text)

    @pyqtSlot(int)
    def _indexChanged(self, value):
        start_index = self.sbStart.value()
        end_index = self.sbEnd.value()

        while start_index > end_index:
            end_index += 1
        with SignalBlocker(self.sbEnd):
            self.sbEnd.setValue(end_index)
        self._update_text()

    @property
    def duplicate_names(self):
        start_index = self.sbStart.value()
        end_index = self.sbEnd.value()
        simple_names = []
        for index in range(start_index, end_index + 1):
            dupe_name = '{}{}'.format(self.leTitle.text(), index)
            simple_names.append(dupe_name)
        return simple_names
