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


from network import Network
from widget import DisplayWidget

from PyQt4.QtGui import QToolButton, QAction


class DisplayCommand(DisplayWidget):
    category = "Slot"
    alias = "Command"

    def __init__(self, box, parent):
        super(DisplayCommand, self).__init__(None)
        self.widget = QToolButton(parent)
        self.current = None
        self.addBox(box)


    def addBox(self, box):
        action = QAction("NO TEXT", self.widget)
        action.box = box
        self.widget.addAction(action)
        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.update)
        self.update()
        return True


    @property
    def boxes(self):
        return [a.box for a in self.widget.actions()]


    def typeChanged(self, box):
        for a in self.widget.actions():
            if a.box is box and a.text() == "NO TEXT":
                a.triggered.disconnect()
                a.triggered.connect(box.execute)
                if a.text() == "NO TEXT":
                    a.setText(box.descriptor.displayedName)


    def update(self):
        for a in self.widget.actions():
            a.setEnabled(a.box.descriptor is not None and
                         a.box.configuration.value.state in
                         a.box.descriptor.allowedStates)
        for a in self.widget.actions():
            if a.isEnabled():
                self.widget.setDefaultAction(a)
                break
        else:
            self.widget.setDefaultAction(self.widget.actions()[0])
