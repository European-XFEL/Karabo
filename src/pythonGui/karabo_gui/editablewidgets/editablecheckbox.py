#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QCheckBox

from karabo.middlelayer import Bool
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import EditableWidget


class EditableCheckBox(EditableWidget):
    category = Bool
    priority = 10
    alias = "Toggle Field"

    def __init__(self, box, parent):
        super(EditableCheckBox, self).__init__(box)

        self.widget = QCheckBox(parent)
        self.widget.setFocusPolicy(Qt.StrongFocus)
        self.widget.stateChanged.connect(self._stateChanged)

    @property
    def value(self):
        return self.widget.checkState() == Qt.Checked

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = False

        checkState = Qt.Checked
        if value in (True, "true", 1):
            checkState = Qt.Checked
        else:
            checkState = Qt.Unchecked
        if value != self.value:
            with SignalBlocker(self.widget):
                self.widget.setCheckState(checkState)

    @pyqtSlot(int)
    def _stateChanged(self, state):
        self.onEditingFinished(state == Qt.Checked)
