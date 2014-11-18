#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["IntLineEdit", "DoubleLineEdit"]


from util import SignalBlocker
from widget import DisplayWidget, EditableWidget

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLineEdit, QDoubleValidator, QPalette, QValidator


class NumberLineEdit(EditableWidget, DisplayWidget):
    category = "Digit"

    def __init__(self, box, parent):
        super(NumberLineEdit, self).__init__(box)
        self.widget = QLineEdit(parent)
        self.widget.setValidator(self.validator)

        # for updates during input, otherwise cursor jumps to end of input
        self.lastCursorPos = 0
        self.normalPalette = self.widget.palette()
        self.errorPalette = QPalette(self.normalPalette)
        self.errorPalette.setColor(QPalette.Text, Qt.red)


    def setReadOnly(self, ro):
        self.widget.setReadOnly(ro)
        if not ro:
            self.widget.textChanged.connect(self.onTextChanged)


    def onTextChanged(self, text):
        self.widget.setPalette(self.normalPalette
                               if self.widget.hasAcceptableInput()
                               else self.errorPalette)
        if self.widget.hasAcceptableInput():
            self.lastCursorPos = self.widget.cursorPosition()
            self.signalEditingFinished.emit(self.boxes[0], self.value)


    def typeChanged(self, box):
        min, max = box.descriptor.getMinMax()
        self.validator.setBottom(min)
        self.validator.setTop(max)


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = 0

        with SignalBlocker(self.widget):
            self.widget.setText("{}".format(value))

        self.widget.setCursorPosition(self.lastCursorPos)


    def validate_value(self):
        """
        This function validates the current value of the widget and returns
        on sucess the value or in failure 0.
        """
        if not self.widget.text():
            return 0
        
        value = self.widget.text()
        #state, _, _ = self.validator.validate(value, 0)
        #if state == QValidator.Invalid or state == QValidator.Intermediate:
        #    value = 0
        return value


class DoubleLineEdit(NumberLineEdit):
    alias = "Float Field"

    def __init__(self, box, parent):
        self.validator = QDoubleValidator(None)
        NumberLineEdit.__init__(self, box, parent)


    @property
    def value(self):
        value = self.validate_value()
        return float(value)


class IntValidator(QValidator):
    def __init__(self, min=None, max=None, parent=None):
        QValidator.__init__(self, parent)
        self.min = min
        self.max = max


    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        if self.min >= 0 and input.startswith('-'):
            return self.Invalid, input, pos

        if self.max < 0 and input.startswith('+'):
            return self.Invalid, input, pos

        if self.min <= int(input) <= self.max:
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos


    def setBottom(self, min):
        self.min = min


    def setTop(self, max):
        self.max = max


class IntLineEdit(NumberLineEdit):
    alias = "Integer Field"

    def __init__(self, box, parent):
        self.validator = IntValidator()
        NumberLineEdit.__init__(self, box, parent)


    @property
    def value(self):
        value = self.validate_value()
        return int(value)
