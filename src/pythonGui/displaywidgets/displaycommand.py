#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
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

__all__ = ["DisplayCommand"]


from manager import Manager
from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayCommand(DisplayWidget):
    category = "Slot"
    alias = "Command"
  
    def __init__(self, box, parent):
        box.configuration.value.state.signalUpdateComponent.connect(
            self.onDeviceStateChanged)
        self.widget = QPushButton(parent)
        super(DisplayCommand, self).__init__(box)


    def typeChanged(self, box):
        self.widget.setText(box.descriptor.displayedName)
        self.allowedStates = box.descriptor.allowedStates
        self.widget.setEnabled(
            box.configuration.value.state.value in self.allowedStates)
        self.widget.clicked.connect(self.onCommandClicked)


    value = None

    def valueChanged(self, key, value, timestamp=None):
        pass


    def onDeviceStateChanged(self, box, value, timestamp):
        self.widget.setEnabled(value in self.allowedStates)


    def onCommandClicked(self):
        for box in self.boxes:
            Manager().executeCommand(dict(path=box.configuration.key,
                                          command=box.path[-1], args=[]))
