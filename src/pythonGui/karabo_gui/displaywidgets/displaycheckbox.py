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

__all__ = ["DisplayCheckBox"]


from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget

from karabo.middlelayer import Bool

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCheckBox


class DisplayCheckBox(DisplayWidget):
    category = Bool
    alias = "Toggle Field"

    
    def __init__(self, box, parent):
        super(DisplayCheckBox, self).__init__(box)
        
        self.widget = QCheckBox(parent)
        self.widget.setEnabled(False)


    @property
    def value(self):
        return self.widget.checkState() == Qt.Checked


    def valueChanged(self, box, value, timestamp=None):
        with SignalBlocker(self.widget):
            self.widget.setCheckState(Qt.Checked if value else Qt.Unchecked)
