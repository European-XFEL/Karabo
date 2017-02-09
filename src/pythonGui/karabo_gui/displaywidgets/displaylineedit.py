#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from karabo.middlelayer import String
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget

from PyQt4.QtGui import QLineEdit


class DisplayLineEdit(DisplayWidget):
    category = String
    priority = 10
    alias = "Text Field"

    def __init__(self, box, parent):
        super(DisplayLineEdit, self).__init__(box)

        self.widget = QLineEdit(parent)
        self.widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
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
