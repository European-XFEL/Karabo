#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QFileDialog, QHBoxLayout, QLineEdit, QToolButton, QWidget)

from karabo.middlelayer import String
import karabo_gui.icons as icons
from karabo_gui.util import getOpenFileName, getSaveFileName, SignalBlocker
from karabo_gui.widget import EditableWidget


class FileSystemPicker(EditableWidget):
    category = String
    priority = 20

    def __init__(self, box, parent):
        super(FileSystemPicker, self).__init__(box)

        self.compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.compositeWidget)
        hLayout.setContentsMargins(0, 0, 0, 0)

        self.lePath = QLineEdit()
        self.lePath.textChanged.connect(self.onEditingFinished)
        self.lePath.setFocusPolicy(Qt.StrongFocus)
        hLayout.addWidget(self.lePath)

        self.tbPath = QToolButton()
        self.tbPath.setStatusTip(self.pickerText)
        self.tbPath.setToolTip(self.pickerText)
        self.tbPath.setIcon(self.buttonIcon)
        self.tbPath.setMaximumSize(25, 25)
        self.tbPath.clicked.connect(self.onButtonClicked)
        hLayout.addWidget(self.tbPath)

        # Needed for updates during input, otherwise cursor jumps to end of
        # input
        self.lastCursorPos = 0

    @property
    def widget(self):
        return self.compositeWidget

    @property
    def value(self):
        return self.lePath.text()

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = ''

        with SignalBlocker(self.lePath):
            self.lePath.setText(value)

        self.lePath.setCursorPosition(self.lastCursorPos)

    @pyqtSlot(str)
    def onEditingFinished(self, value):
        self.lastCursorPos = self.lePath.cursorPosition()
        EditableWidget.onEditingFinished(self, value)

    @pyqtSlot()
    def onButtonClicked(self):
        path = self.picker()
        if path:
            self.lePath.setText(path)


class EditableDirectory(FileSystemPicker):
    displayType = "directory"
    alias = "Directory"
    buttonIcon = icons.load
    pickerText = 'Select directory'

    def picker(self):
        return QFileDialog.getExistingDirectory(None, self.pickerText)


class EditableFileIn(FileSystemPicker):
    displayType = "fileIn"
    alias = "File In"
    buttonIcon = icons.filein
    pickerText = 'Select input file'

    def picker(self):
        return getOpenFileName(caption=self.pickerText)


class EditableFileOut(FileSystemPicker):
    displayType = "fileOut"
    alias = "File Out"
    buttonIcon = icons.fileout
    pickerText = 'Select output file'

    def picker(self):
        return getSaveFileName(caption=self.pickerText)
