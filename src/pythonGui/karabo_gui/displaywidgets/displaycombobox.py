#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QComboBox

from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget


class DisplayComboBox(DisplayWidget):
    alias = "Selection Field"

    def __init__(self, box, parent):
        super(DisplayComboBox, self).__init__(box)

        self.widget = QComboBox(parent)
        self.widget.setFrame(False)
        self.widget.setEnabled(False)

    @classmethod
    def isCompatible(cls, box, readonly):
        options = getattr(box.descriptor, "options", None)
        return readonly and options is not None

    def typeChanged(self, box):
        with SignalBlocker(self.widget):
            self.widget.clear()
            self.widget.addItems([str(o) for o in box.descriptor.options])

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
