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


from listedit import ListEdit
from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import QLabel, QFrame, QDialog

import numbers
import numpy


class Label(QLabel):
    valueChanged = pyqtSignal(list)

    def __init__(self, parent):
        super(Label, self).__init__(parent)
        self.setAcceptDrops(True)
        self.value = [ ]


    def setValue(self, value):
        l = [ ]
        for v in value[:10]:
            if (isinstance(v, (numbers.Real, numpy.floating)) and
                    not isinstance(v, numbers.Integral)):
                l.append("{:.6}".format(v))
            else:
                l.append("{}".format(v))
        if len(value) > 10:
            l.append("...")

        with SignalBlocker(self):
            self.setText(", ".join(l))
        self.value = value


    def mouseDoubleClickEvent(self, event) :
        listEdit = ListEdit(self.valueType, True, self.value)
        listEdit.setTexts("Add", "&Value", "Edit")

        if listEdit.exec_() == QDialog.Accepted:
            values = [listEdit.getListElementAt(i)
                      for i in xrange(listEdit.getListCount())]
            self.value = values
            self.valueChanged.emit(values)


class EditableList(EditableWidget):
    category = "List"
    alias = "Plot"

    def __init__(self, box, parent):
        self.widget = Label(parent)
        self.widget.setMinimumWidth(160)
        self.widget.setMaximumHeight(24)
        self.widget.setFrameStyle(QFrame.Box)
        self.widget.valueChanged.connect(self.onEditingFinished)
        super(EditableList, self).__init__(box)

    @property
    def value(self):
        return self.widget.value


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        self.widget.setValue(value)


    def typeChanged(self, box):
        self.widget.valueType = box.descriptor


    def onEditingFinished(self, value):
        self.boxes[0].set(value)
        self.signalEditingFinished.emit(self.boxes[0], value)
