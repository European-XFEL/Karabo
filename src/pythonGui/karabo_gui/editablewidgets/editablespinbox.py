#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QSpinBox

from karabo.middlelayer import Integer
from karabo_gui.displaywidgets.unitlabel import add_unit_label
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget, EditableWidget


class EditableSpinBox(EditableWidget, DisplayWidget):
    category = Integer
    alias = "Integer Spin Box"

    def __init__(self, box, parent):
        super(EditableSpinBox, self).__init__(box)
        self._internal_widget = QSpinBox(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)

    def setReadOnly(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.installEventFilter(self)
            self._internal_widget.valueChanged.connect(self.onEditingFinished)

    def eventFilter(self, object, event):
        # Block wheel event on QSpinBox
        return event.type() == QEvent.Wheel and object is self._internal_widget

    @property
    def value(self):
        return self._internal_widget.value()

    def typeChanged(self, box):
        rmin, rmax = box.descriptor.getMinMax()
        self._internal_widget.setRange(max(-0x80000000, rmin),
                                       min(0x7fffffff, rmax))

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)

        if value is None:
            value = 0

        with SignalBlocker(self._internal_widget):
            self._internal_widget.setValue(value)
