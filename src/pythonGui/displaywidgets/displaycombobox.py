#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayComboBox"]


from util import SignalBlocker
from widget import DisplayWidget

from PyQt4.QtGui import QComboBox


class DisplayComboBox(DisplayWidget):
    alias = "Selection Field"
    
    def __init__(self, box, parent):
        super(DisplayComboBox, self).__init__(box)
        
        self.widget = QComboBox(parent)
        self.widget.setFrame(False)
        self.widget.setEnabled(False)


    @classmethod
    def isCompatible(cls, box, readonly):
        return readonly and box.descriptor.options is not None


    def typeChanged(self, box):
        with SignalBlocker(self.widget):
            self.widget.clear()
            self.widget.addItems(box.descriptor.options)


    @property
    def value(self):
        return self.widget.currentText()


    def addItems(self, texts):
        with SignalBlocker(self.widget):
            self.widget.addItems(texts)


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return

        index = self.widget.findText(str(value))
        if index < 0:
            return
        
        if value != self.value:
            with SignalBlocker(self.widget):
                self.widget.setCurrentIndex(index)
