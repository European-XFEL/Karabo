#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget

from karabo.middlelayer import Bool

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCheckBox


class DisplayCheckBox(DisplayWidget):
    category = Bool
    alias = "Toggle Field"
    priority = 10

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
