#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import QLineEdit, QDoubleValidator, QPalette, QValidator

from karabo.middlelayer import Integer, Number
from karabo_gui.displaywidgets.unitlabel import add_unit_label
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget, EditableWidget


class NumberLineEdit(EditableWidget, DisplayWidget):
    def __init__(self, box, parent):
        super(NumberLineEdit, self).__init__(box)
        self._internal_widget = QLineEdit(parent)
        self._internal_widget.setValidator(self.validator)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)
        self.normalPalette = self._internal_widget.palette()
        self.errorPalette = QPalette(self.normalPalette)
        self.errorPalette.setColor(QPalette.Text, Qt.red)

    def setReadOnly(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.textChanged.connect(self.onTextChanged)

    @pyqtSlot(str)
    def onTextChanged(self, text):
        palette = (self.normalPalette
                   if self._internal_widget.hasAcceptableInput()
                   else self.errorPalette)
        self._internal_widget.setPalette(palette)
        if self._internal_widget.hasAcceptableInput():
            self.signalEditingFinished.emit(self.boxes[0], self.value)

    def typeChanged(self, box):
        min, max = box.descriptor.getMinMax()
        self.validator.setBottom(min)
        self.validator.setTop(max)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)

        if value is None:
            value = 0

        if not self._internal_widget.hasFocus():
            with SignalBlocker(self._internal_widget):
                self._internal_widget.setText("{}".format(value))

    def validate_value(self):
        """
        This function validates the current value of the widget and returns
        on sucess the value or in failure 0.
        """
        if not self._internal_widget.text():
            return 0

        value = self._internal_widget.text()
        state, _, _ = self.validator.validate(value, 0)
        if state == QValidator.Invalid or state == QValidator.Intermediate:
            value = 0
        return value


class DoubleLineEdit(NumberLineEdit):
    category = Number
    priority = 10
    alias = "Float Field"

    def __init__(self, box, parent):
        self.validator = QDoubleValidator(None)
        NumberLineEdit.__init__(self, box, parent)

    @property
    def value(self):
        return float(self.validate_value())


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

        if self.min is not None and self.min >= 0 and input.startswith('-'):
            return self.Invalid, input, pos

        if self.max is not None and self.max < 0 and input.startswith('+'):
            return self.Invalid, input, pos

        if ((self.min is None or self.min <= int(input)) and
                (self.max is None or int(input) <= self.max)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, min):
        self.min = min

    def setTop(self, max):
        self.max = max


class IntLineEdit(NumberLineEdit):
    category = Integer
    priority = 10
    alias = "Integer Field"

    def __init__(self, box, parent):
        self.validator = IntValidator()
        NumberLineEdit.__init__(self, box, parent)

    @property
    def value(self):
        return int(self.validate_value())
