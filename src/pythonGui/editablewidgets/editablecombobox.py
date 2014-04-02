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

__all__ = ["EditableComboBox"]


from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableComboBox(EditableWidget):
    category = "Selection"
    alias = "Selection Field"

    def __init__(self, box, parent):
        super(EditableComboBox, self).__init__(box)
        
        self.__comboBox = QComboBox(parent)
        self.__comboBox.setFrame(False)

        self.__comboBox.installEventFilter(self)
        self.__comboBox.currentIndexChanged[str].connect(self.onEditingFinished)
        box.addWidget(self)


    def typeChanged(self, box):
        with SignalBlocker(self.__comboBox):
            self.__comboBox.clear()
            self.__comboBox.addItems(box.descriptor.options)


    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        return event.type() == QEvent.Wheel and object == self.__comboBox


    @property
    def widget(self):
        return self.__comboBox


    @property
    def value(self):
        try:
            if self.valueType == "int":
                return int(self.__comboBox.currentText())
            elif self.valueType in ("float", "double"):
                return float(self.__comboBox.currentText())
            return self.__comboBox.currentText()
        except Exception, e:
            print e


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            return

        index = self.__comboBox.findText(str(value))
        if index < 0:
            return

        self.__comboBox.blockSignals(True)
        self.__comboBox.setCurrentIndex(index)
        self.__comboBox.blockSignals(False)

        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)
