#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class EditableWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["EditableList"]


import icons
from listedit import ListEdit
from util import SignalBlocker
from widget import EditableWidget, DisplayWidget

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import QDialog, QFrame, QHBoxLayout, QLineEdit, QToolButton, QWidget

import numbers
import numpy


class EditableList(EditableWidget, DisplayWidget):
    category = "List"
    alias = "List"

    def __init__(self, box, parent):
        super(EditableList, self).__init__(box)
        
        self.compositeWidget = QWidget(parent)
        self.hLayout = QHBoxLayout(self.compositeWidget)
        self.hLayout.setContentsMargins(0,0,0,0)

        self.leList = QLineEdit()
        self.hLayout.addWidget(self.leList)

        self.valueList = []
        
        # Needed for updates during input, otherwise cursor jumps to end of input
        self.lastCursorPos = 0


    def setReadOnly(self, ro):
        self.leList.setReadOnly(ro)
        if ro:
            return

        self.leList.textChanged.connect(self.onEditingFinished)

        text = "Edit"
        self.tbEdit = QToolButton()
        self.tbEdit.setStatusTip(text)
        self.tbEdit.setToolTip(text)
        self.tbEdit.setIcon(icons.edit)
        self.tbEdit.setMaximumSize(25,25)
        self.tbEdit.clicked.connect(self.onEditClicked)
        self.hLayout.addWidget(self.tbEdit)


    @property
    def widget(self):
        return self.compositeWidget


    @property
    def value(self):
        return self.valueList


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = []

        self.valueList = value

        with SignalBlocker(self.leList):
            self.leList.setText(box.descriptor.toString(value))
        
        self.leList.setCursorPosition(self.lastCursorPos)


    def onEditingFinished(self, text):
        self.lastCursorPos = self.leList.cursorPosition()
        if text:
            self.valueList = text.split(',')
        else:
            self.valueList = []
        EditableWidget.onEditingFinished(self, self.valueList)


    def onEditClicked(self):
        listEdit = ListEdit(self.valueType, True, self.valueList)
        listEdit.setTexts("Add", "&Value", "Edit")

        if listEdit.exec_() == QDialog.Accepted:
            values = [listEdit.getListElementAt(i)
                      for i in xrange(listEdit.getListCount())]

            self.leList.setText(self.boxes[0].descriptor.toString(values))
            

