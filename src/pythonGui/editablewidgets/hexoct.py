#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["Hexadecimal"]


from util import SignalBlocker
from widget import DisplayWidget, EditableWidget

from PyQt4.QtGui import QLineEdit
from numpy import log2


class Hexadecimal(EditableWidget, DisplayWidget):
    category = "Digit"
    alias = "Hexadecimal"

    def __init__(self, box, parent):
        super(Hexadecimal, self).__init__(box)
        self.widget = QLineEdit(parent)


    def setReadOnly(self, ro):
        self.widget.setReadOnly(ro)
        if not ro:
            self.widget.textChanged.connect(self.onEditingFinished)


    @property
    def value(self):
        if self.widget.text() not in ('', '-'):
            return int(self.widget.text(), base=16)
        else:
            return 0


    def typeChanged(self, box):
        rmin, rmax = box.descriptor.getMinMax()
        mask = 'h' * (log2(max(abs(rmax), abs(rmin))) // 4 + 1)
        if rmin < 0:
            mask = "#" + mask
        self.widget.setInputMask(mask)


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        with SignalBlocker(self.widget):
            self.widget.setText("{:x}".format(value))


    def onEditingFinished(self, value):
        EditableWidget.onEditingFinished(self, self.value)
