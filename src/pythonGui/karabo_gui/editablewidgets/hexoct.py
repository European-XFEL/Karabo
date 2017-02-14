#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from numpy import log2
from PyQt4.QtGui import QLineEdit

from karabo.middlelayer import Integer
from karabo_gui.displaywidgets.unitlabel import add_unit_label
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget, EditableWidget


class Hexadecimal(EditableWidget, DisplayWidget):
    category = Integer
    alias = "Hexadecimal"

    def __init__(self, box, parent):
        super(Hexadecimal, self).__init__(box)
        self._internal_widget = QLineEdit(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)

    def setReadOnly(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.textChanged.connect(self.onEditingFinished)

    @property
    def value(self):
        if self._internal_widget.text() not in ('', '-'):
            return int(self._internal_widget.text(), base=16)
        else:
            return 0

    def typeChanged(self, box):
        rmin, rmax = box.descriptor.getMinMax()
        mask = 'h' * int(log2(max(abs(rmax), abs(rmin))) // 4 + 1)
        if rmin < 0:
            mask = "#" + mask
        self._internal_widget.setInputMask(mask)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setText("{:x}".format(value))

    def onEditingFinished(self, value):
        EditableWidget.onEditingFinished(self, self.value)
