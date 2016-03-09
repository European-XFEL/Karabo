#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 28, 2012
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

__all__ = ["EditableChoiceElement"]


from karabo_gui.widget import EditableWidget
from karabo.api_2 import Hash
from schema import Schema, Descriptor, ChoiceOfNodes
from karabo_gui.util import SignalBlocker

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QComboBox


class EditableChoiceElement(EditableWidget):
    category = ChoiceOfNodes
    alias = "Choice Element"

    def __init__(self, box, parent):
        super(EditableChoiceElement, self).__init__(box)
        
        self.widget = QComboBox(parent)
        self.widget.setFrame(False)

        self.widget.installEventFilter(self)
        self.widget.currentIndexChanged.connect(self.onEditingFinished)

        self.childItemList = None  # set by schema.ChoiceOfNodes.item

    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        if event.type() == QEvent.Wheel and object == self.widget:
            return True
        return False


    def typeChanged(self, box):
        for k, v in box.descriptor.dict.items():
            if isinstance(v, Descriptor):
                self.widget.addItem(v.displayedName, k)

    @property
    def value(self):
        index = self.widget.currentIndex()
        data = self.widget.itemData(index)
        return data


    def _updateChoiceItems(self, index):
        for item in self.childItemList:
            item.setHidden(True)
            item.updateNeeded = False

        selectedItem = self.childItemList[index]
        selectedItem.setHidden(False)
        selectedItem.updateNeeded = True
        self._r_updateChildItems(selectedItem)


    def _r_updateChildItems(self, parentItem):
        for i in range(parentItem.childCount()):
            childItem = parentItem.child(i)
            if parentItem.updateNeeded:
                if parentItem.isChoiceElement:
                    if parentItem.defaultValue in (None, childItem.box.path[-1]):
                        childItem.updateNeeded = True
                        childItem.setHidden(False)
                    else:
                        childItem.updateNeeded = False
                else:
                    childItem.updateNeeded = True
                    childItem.setHidden(False)
            else:
                childItem.updateNeeded = False
            self._r_updateChildItems(childItem)


    def valueChanged(self, box, value, timestamp=None):
        if not isinstance(value, str):
            value = box.current

        index = self.widget.findData(value)
        if index < 0:
            return

        with SignalBlocker(self.widget):
            self.widget.setCurrentIndex(index)
        if self.childItemList is not None:
            self._updateChoiceItems(index)


    def onEditingFinished(self, index):
        if self.childItemList is not None:
            self._updateChoiceItems(index)
        EditableWidget.onEditingFinished(self, self.value)


    def copy(self, item):
        copyWidget = EditableChoiceElement(item=item)

        if not self.item.isChoiceElement:
            for i in range(self.widget.count()):
                copyWidget.comboBox.addItem(self.widget.itemText(i), self.widget.itemData(i))

        return copyWidget
