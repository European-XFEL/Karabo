from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import QLineEdit, QDoubleValidator, QPalette, QValidator

from karabo.api_2 import Integer
from karabo_gui.util import SignalBlocker
from .widget import AttributeWidget


class IntValidator(QValidator):
    def __init__(self, parent=None):
        super(IntValidator, self).__init__(parent)

    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


class NumberAttributeEditor(AttributeWidget):
    def __init__(self, box, attributeName, parent=None):
        super(NumberAttributeEditor, self).__init__(box, attributeName)
        self.widget = QLineEdit(parent)

        valueType = type(box.descriptor)
        if issubclass(valueType, Integer):
            self.validator = IntValidator(parent=self.widget)
            self._value_cast = int
        else:
            self.validator = QDoubleValidator(parent=self.widget)
            self._value_cast = float

        self.widget.setValidator(self.validator)
        self.widget.textChanged.connect(self.onTextChanged)

        self.normalPalette = self.widget.palette()
        self.errorPalette = QPalette(self.normalPalette)
        self.errorPalette.setColor(QPalette.Text, Qt.red)


    @pyqtSlot(str)
    def onTextChanged(self, text):
        self.widget.setPalette(self.normalPalette
                               if self.widget.hasAcceptableInput()
                               else self.errorPalette)
        if self.widget.hasAcceptableInput():
            self.signalEditingFinished.emit(self.boxes[0], self.value)

    def attributeValueChanged(self, value):
        if value is None:
            value = 0

        if not self.widget.hasFocus():
            with SignalBlocker(self.widget):
                self.widget.setText("{}".format(value))

    def validate_value(self):
        """ This function validates the current value of the widget and returns
        on success the value or in failure 0.
        """
        if not self.widget.text():
            return 0

        value = self.widget.text()
        state, _, _ = self.validator.validate(value, 0)
        if state == QValidator.Invalid or state == QValidator.Intermediate:
            value = 0
        return value

    @property
    def value(self):
        return self._value_cast(self.validate_value())
