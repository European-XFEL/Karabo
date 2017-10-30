#############################################################################
# Author: <alessandro.silenzi@xfel.eu>
# Created on October 26, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QProgressBar

from karabo.middlelayer import Simple
from karabo_gui import messagebox
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.widget import DisplayWidget


def _clamp(val, min_val, max_val):
    return min(max_val, max(min_val, val))


class DisplayProgressBar(DisplayWidget):
    category = Simple
    alias = "Progress Bar"

    def __init__(self, model, box, parent):
        super(DisplayProgressBar, self).__init__(box)
        self.widget = QProgressBar(parent)
        self.widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.model = model
        self.value = None
        orient_action = QAction("Change Orientation", self.widget)
        orient_action.triggered.connect(self.orientation_action)
        self.widget.addAction(orient_action)
        self._setVertical(self.model.is_vertical)

    @pyqtSlot()
    def orientation_action(self):
        self._setVertical(not self.model.is_vertical)

    def typeChanged(self, box):
        desc = box.descriptor

        bad_limits = ((desc.minInc is None and desc.minExc is None) or
                      (desc.maxInc is None and desc.maxExc is None))
        if bad_limits:
            msg = ('No proper configuration detected.\n Please define min '
                   'and max thresholds.')
            messagebox.show_warning(msg, title='Wrong property configuration')
        self._eval_limits(box)

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return
        value = _clamp(value, self.widget.minimum(), self.widget.maximum())
        if value != self.value:
            self.value = value
            self.widget.setValue(self.value)

    def _eval_limits(self, box):
        desc = box.descriptor
        # min is set to desc.minInc if none else desc.minExc else 0
        min = desc.minInc or desc.minExc or 0
        self.widget.setMinimum(min)
        # max is set to desc.maxInc if none else desc.maxExc else 0
        max = desc.maxInc or desc.maxExc or 0
        self.widget.setMaximum(max)

    def _setVertical(self, vertical):
        self.model.is_vertical = vertical
        if vertical:
            self.widget.setOrientation(Qt.Vertical)
        else:
            self.widget.setOrientation(Qt.Horizontal)
