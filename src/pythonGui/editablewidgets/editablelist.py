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
from widget import EditableWidget

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import QDialog, QFrame, QHBoxLayout, QLineEdit, QToolButton, QWidget

import numbers
import numpy


class EditableList(EditableWidget):
    category = "List"
    alias = "Plot"

    def __init__(self, box, parent):
        super(EditableList, self).__init__(box)
        
        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)
        
        self.leList = QLineEdit()
        self.leList.textChanged.connect(self.onEditingFinished)
        hLayout.addWidget(self.leList)

        text = "Edit"
        self.tbEdit = QToolButton()
        self.tbEdit.setStatusTip(text)
        self.tbEdit.setToolTip(text)
        self.tbEdit.setIcon(icons.edit)
        self.tbEdit.setMaximumSize(25,25)
        self.tbEdit.clicked.connect(self.onEditClicked)
        hLayout.addWidget(self.tbEdit)
        
        self.valueList = []


    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.valueList


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = []

        self.valueList = value

        with SignalBlocker(self.leList):
            self.leList.setText(box.descriptor.toString(value))


    def onEditingFinished(self, text):
        self.valueList = text.split(',')
        EditableWidget.onEditingFinished(self, self.valueList)


    def onEditClicked(self):
        listEdit = ListEdit(self.valueType, True, self.valueList)
        listEdit.setTexts("Add", "&Value", "Edit")

        if listEdit.exec_() == QDialog.Accepted:
            values = [listEdit.getListElementAt(i)
                      for i in xrange(listEdit.getListCount())]
            self.valueList = values
            EditableWidget.onEditingFinished(self, self.valueList)

