#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class EditableWidget.
"""

__all__ = ["EditableFileIn"]


import icons
from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtGui import (QFileDialog, QHBoxLayout, QLineEdit, QToolButton,
                         QWidget)


class EditableFileIn(EditableWidget):
    category = "String"
    alias = "File In"

    def __init__(self, box, parent):
        super(EditableFileIn, self).__init__(box)
        
        self.compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.lePath = QLineEdit()
        self.lePath.textChanged.connect(self.onEditingFinished)
        hLayout.addWidget(self.lePath)
        
        text = "Select input file"
        self.tbPath = QToolButton()
        self.tbPath.setStatusTip(text)
        self.tbPath.setToolTip(text)
        self.tbPath.setIcon(icons.filein)
        self.tbPath.setMaximumSize(25, 25)
        self.tbPath.clicked.connect(self.onFileInClicked)
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
        self.onEditingFinished(value)


    def onEditingFinished(self, value):
        self.lastCursorPos = self.lePath.cursorPosition()
        EditableWidget.onEditingFinished(self, value)


    def onFileInClicked(self):
        fileIn = QFileDialog.getOpenFileName(None, "Select input file")
        if not fileIn:
            return

        self.lePath.setText(fileIn)

