#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class EditableWidget.
"""

__all__ = ["EditableDirectory"]


import karabo_gui.icons as icons
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import EditableWidget

from karabo.middlelayer import String

from PyQt4.QtGui import (QFileDialog, QHBoxLayout, QLineEdit, QToolButton,
                         QWidget)


class EditableDirectory(EditableWidget):
    category = String
    priority = 20
    displayType = "directory"
    alias = "Directory"

    def __init__(self, box, parent):
        super(EditableDirectory, self).__init__(box)
        
        self.compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)
        
        self.lePath = QLineEdit()
        self.lePath.textChanged.connect(self.onEditingFinished)
        hLayout.addWidget(self.lePath)
        
        text = "Select directory"
        self.tbPath = QToolButton()
        self.tbPath.setStatusTip(text)
        self.tbPath.setToolTip(text)
        self.tbPath.setIcon(icons.open)
        self.tbPath.setMaximumSize(25, 25)
        self.tbPath.clicked.connect(self.onDirectoryClicked)
        hLayout.addWidget(self.tbPath)

        # Needed for updates during input, otherwise cursor jumps to end of input
        self.lastCursorPos = 0


    @property
    def widget(self):
        return self.compositeWidget


    @property
    def value(self):
        return self.lePath.text()


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = ""

        with SignalBlocker(self.lePath):
            self.lePath.setText(value)

        self.lePath.setCursorPosition(self.lastCursorPos)


    def onEditingFinished(self, value):
        self.lastCursorPos = self.lePath.cursorPosition()
        EditableWidget.onEditingFinished(self, value)


    def onDirectoryClicked(self):
        directory = QFileDialog.getExistingDirectory(None, "Select directory")
        if not directory:
            return

        self.lePath.setText(directory)

