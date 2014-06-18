#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["EditableDoubleSpinBox"]

from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLineEdit, QDoubleValidator, QPalette


class EditableDoubleSpinBox(EditableWidget):
    category = "Digit"
    alias = "Float Field"

    def __init__(self, box, parent):
        super(EditableDoubleSpinBox, self).__init__(box)
        self.widget = QLineEdit(parent)
        self.validator = QDoubleValidator(self.widget)
        self.widget.setValidator(self.validator)
        self.widget.editingFinished.connect(self.onEditingFinished)
        self.widget.textChanged.connect(self.onTextChanged)

        # Needed for updates during input, otherwise cursor jumps to end of input
        self.lastCursorPos = 0
        self.normalPalette = self.widget.palette()
        self.errorPalette = QPalette(self.normalPalette)
        self.errorPalette.setColor(QPalette.Text, Qt.red)


    def onTextChanged(self, text):
        self.widget.setPalette(self.normalPalette
                                     if self.widget.hasAcceptableInput()
                                     else self.errorPalette)


    def typeChanged(self, box):
        min, max = box.descriptor.getMinMax()
        self.validator.setBottom(min)
        self.validator.setTop(max)


    @property
    def value(self):
        return float(self.widget.text())


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0.0

        with SignalBlocker(self.widget):
            self.widget.setText("{}".format(value))

        self.widget.setCursorPosition(self.lastCursorPos)

        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


    def onEditingFinished(self):
        self.lastCursorPos = self.widget.cursorPosition()
        self.signalEditingFinished.emit(self.boxes[0], self.value)
