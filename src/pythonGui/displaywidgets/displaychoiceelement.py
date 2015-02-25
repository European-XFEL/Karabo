#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 6, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["DisplayChoiceElement"]


from schema import ChoiceOfNodes
from widget import DisplayWidget

from PyQt4.QtGui import QComboBox


class DisplayChoiceElement(DisplayWidget):
    category = ChoiceOfNodes
    alias = "Choice Element"

    def __init__(self, box, parent):
        super().__init__(box)

        self.widget = QComboBox(parent)
        self.widget.setFrame(False)
        self.widget.currentIndexChanged.connect(self.onEditingFinished)


    @property
    def value(self):
        return self.widget.currentText()


    def typeChanged(self, box):
        print(box, box.descriptor)


    def valueChanged(self, box, value, timestamp=None):
        if not isinstance(value, str):
            return

        index = self.widget.findText(value)
        if index < 0:
            return

        self.widget.setCurrentIndex(index)


    def onEditingFinished(self, index):
        if index > -1 and index < len(self.childItemList):
            for i in range(len(self.childItemList)):
                item = self.childItemList[i]
                item.setHidden(True)
                item.updateNeeded = False
