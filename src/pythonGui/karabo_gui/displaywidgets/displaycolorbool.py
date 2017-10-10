#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4.QtCore import QByteArray, pyqtSlot
from PyQt4.QtGui import QAction
from PyQt4.QtSvg import QSvgWidget

from karabo.middlelayer import Bool, State
from karabo_gui import icons
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.icons.statefulicons.color_change_icon import (
    get_color_change_icon)
from karabo_gui.widget import DisplayWidget


class DisplayColorBool(DisplayWidget):
    category = Bool
    alias = "Switch Bool"

    def __init__(self, box, parent):
        super(DisplayColorBool, self).__init__(box)
        self.invert = False
        self.widget = QSvgWidget(parent)
        self.widget.setMaximumSize(24, 24)
        self.widget.resize(20, 20)

        logicAction = QAction("Invert color logic", self.widget)
        logicAction.triggered.connect(self.logic_action)
        self.widget.addAction(logicAction)

        path = op.join(op.dirname(icons.__file__), 'switch-bool.svg')
        self.icon = get_color_change_icon(path)

    def valueChanged(self, box, value, timestamp=None):
        if not self.invert:
            color_state = State.ACTIVE if value else State.PASSIVE
        else:
            color_state = State.PASSIVE if value else State.ACTIVE

        svg = self.icon.with_color(STATE_COLORS[color_state])
        self.widget.load(QByteArray(svg))

    @pyqtSlot()
    def logic_action(self):
        self._setInvert(not self.invert)

    def _setInvert(self, value):
        self.invert = value
        self.valueChanged(self.boxes[0], self.boxes[0].value)
