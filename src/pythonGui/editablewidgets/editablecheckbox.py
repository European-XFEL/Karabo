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

__all__ = ["EditableCheckBox"]


from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCheckBox


class EditableCheckBox(EditableWidget):
    category = "Switch"
    alias = "Toggle Field"

    def __init__(self, box, parent):
        super(EditableCheckBox, self).__init__(box)

        self.widget = QCheckBox(parent)
        self.widget.stateChanged.connect(self.onEditingFinished)


    @property
    def value(self):
        return self.widget.checkState() == Qt.Checked


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = False

        checkState = Qt.Checked
        if value in (True, "true", 1):
            checkState = Qt.Checked
        else:
            checkState = Qt.Unchecked
        if value != self.value:
            with SignalBlocker(self.widget):
                self.widget.setCheckState(checkState)

        self.onEditingFinished(checkState)
