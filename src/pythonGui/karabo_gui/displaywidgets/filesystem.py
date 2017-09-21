#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLineEdit

from karabo.middlelayer import String
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget


class _FilesystemDisplay(DisplayWidget):
    category = String

    def __init__(self, box, parent):
        super(_FilesystemDisplay, self).__init__(box)

        self.widget = QLineEdit()
        self.widget.setReadOnly(True)
        self.widget.setFocusPolicy(Qt.NoFocus)

    @property
    def value(self):
        return self.widget.text()

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return

        if value != self.value:
            with SignalBlocker(self.widget):
                self.widget.setText(value)


class DisplayDirectory(_FilesystemDisplay):
    displayType = "directory"
    alias = "Directory"


class DisplayFileIn(_FilesystemDisplay):
    displayType = "fileIn"
    alias = "File In"


class DisplayFileOut(_FilesystemDisplay):
    displayType = "fileOut"
    alias = "File Out"
