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

__all__ = ["DisplayLineEdit"]


from karabo.hashtypes import String
from util import SignalBlocker
from widget import DisplayWidget

from PyQt4.QtGui import QLineEdit


class DisplayLineEdit(DisplayWidget):
    category = String
    priority = 10
    alias = "Text Field"

    def __init__(self, box, parent):
        super(DisplayLineEdit, self).__init__(box)
        
        self.widget = QLineEdit(parent)
        self.widget.setMinimumSize(160, 24)
        self.widget.setReadOnly(True)


    @property
    def value(self):
        return self.widget.text()


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return
        
        if value != self.value:
            with SignalBlocker(self.widget):
                self.widget.setText(value)
