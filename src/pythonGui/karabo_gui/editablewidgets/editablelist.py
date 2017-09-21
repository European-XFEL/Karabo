#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QDialog, QHBoxLayout, QLineEdit, QToolButton, QWidget

from karabo.middlelayer import Vector
import karabo_gui.icons as icons
from karabo_gui.listedit import ListEdit
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import EditableWidget, DisplayWidget


class EditableList(EditableWidget, DisplayWidget):
    category = Vector
    priority = 10
    alias = "List"

    def __init__(self, box, parent):
        super(EditableList, self).__init__(box)

        self.compositeWidget = QWidget(parent)
        self.hLayout = QHBoxLayout(self.compositeWidget)
        self.hLayout.setContentsMargins(0, 0, 0, 0)

        self.leList = QLineEdit()
        self.hLayout.addWidget(self.leList)

        self.valueList = []

        # Needed for updates during input, otherwise cursor jumps to end of
        # input
        self.lastCursorPos = 0

    def setReadOnly(self, ro):
        self.leList.setReadOnly(ro)
        self.leList.setFocusPolicy(Qt.NoFocus if ro else Qt.StrongFocus)
        if ro:
            return

        self.leList.textChanged.connect(self.onEditingFinished)

        text = "Edit"
        self.tbEdit = QToolButton()
        self.tbEdit.setStatusTip(text)
        self.tbEdit.setToolTip(text)
        self.tbEdit.setIcon(icons.edit)
        self.tbEdit.setMaximumSize(25, 25)
        self.tbEdit.setFocusPolicy(Qt.NoFocus)
        self.tbEdit.clicked.connect(self.onEditClicked)
        self.hLayout.addWidget(self.tbEdit)

    @property
    def widget(self):
        return self.compositeWidget

    @property
    def value(self):
        return self.valueList

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = []

        self.valueList = value

        with SignalBlocker(self.leList):
            self.leList.setText(box.descriptor.toString(value))

        self.leList.setCursorPosition(self.lastCursorPos)

    @pyqtSlot(str)
    def onEditingFinished(self, text):
        self.lastCursorPos = self.leList.cursorPosition()
        if text:
            self.valueList = text.split(',')
        else:
            self.valueList = []
        EditableWidget.onEditingFinished(self, self.valueList)

    @pyqtSlot()
    def onEditClicked(self):
        listEdit = ListEdit(self.boxes[0].descriptor, True, self.valueList)
        listEdit.setTexts("Add", "&Value", "Edit")

        if listEdit.exec_() == QDialog.Accepted:
            values = [listEdit.getListElementAt(i)
                      for i in range(listEdit.getListCount())]

            self.leList.setText(self.boxes[0].descriptor.toString(values))
