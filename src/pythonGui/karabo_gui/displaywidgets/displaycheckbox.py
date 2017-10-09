#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4.QtSvg import QSvgWidget

from karabo_gui import icons
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import Bool

ICONS = op.dirname(icons.__file__)
CHECKED = op.join(ICONS, "checkbox-checked.svg")
UNCHECKED = op.join(ICONS, "checkbox-unchecked.svg")


class DisplayCheckBox(DisplayWidget):
    category = Bool
    alias = "Toggle Field"
    priority = 10

    def __init__(self, box, parent):
        super(DisplayCheckBox, self).__init__(box)

        self.widget = QSvgWidget(parent)
        self.widget.setFixedSize(20, 20)
        self.value = False

    def valueChanged(self, box, value, timestamp=None):
        svg = CHECKED if value else UNCHECKED
        self.widget.load(svg)
        self.value = value
