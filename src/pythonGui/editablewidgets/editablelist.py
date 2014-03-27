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
from PyQt4.QtGui import QLabel, QFrame

import numbers
import numpy


class Label(QLabel):
    def __init__(self, parent):
        super(Label, self).__init__(parent)
        self.setAcceptDrops(True)


    def setValue(self, box):
        self.box = box

        l = [ ]
        for v in self.box.value[:10]:
            if isinstance(v, (numbers.Real, numpy.floating)):
                l.append("{:.6}".format(v))
            else:
                l.append("{}".format(v))
        if len(self.box.value) > 10:
            l.append("...")

        with SignalBlocker(self):
            self.setText(", ".join(l))


    def mouseDoubleClickEvent(self, event) :

        listEdit = ListEdit(self.box.descriptor.valueType, True, self.box.value)
        listEdit.setTexts("Add", "&Value", "Edit")

        if listEdit.exec_() == QDialog.Accepted:
            values = [listEdit.getListElementAt(i)
                      for i in xrange(listEdit.getListCount())]
            self.box.set(values)


class EditableList(EditableWidget):
    category = "List"
    alias = "Histogram"

    def __init__(self, box, parent):
        super(EditableList, self).__init__(box)
        
        self.widget = Label(parent)
        self.widget.setMinimumWidth(160)
        self.widget.setMaximumHeight(24)
        self.widget.setFrameStyle(QFrame.Box)
        box.addWidget(self)


    @property
    def value(self):
        return self.widget.box.value


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        self.widget.setValue(box)


    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.keys[0], value)
