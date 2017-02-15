
import traceback
from PyQt4.QtGui import QAction, QInputDialog, QLineEdit, QMessageBox

from karabo.middlelayer import String, Simple
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget
from .unitlabel import add_unit_label


class Evaluator(DisplayWidget):
    category = String, Simple
    alias = "Evaluate Expression"

    globals = {}
    exec("from numpy import *", globals)

    def __init__(self, box, parent):
        super(Evaluator, self).__init__(box)

        self._internal_widget = QLineEdit(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.setReadOnly(True)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)
        self.text = "x"
        self.value = None

        action = QAction("Change expression...", self.widget)
        self.widget.addAction(action)
        action.triggered.connect(self.onChangeExpression)

    def function(self, x):
        """ this is just a placeholder """
        return x

    def onChangeExpression(self):
        text, ok = QInputDialog.getText(self.widget, "Enter Expression",
                                        "f(x) = ", text=self.text)
        if ok:
            self.setText(text)

    def setText(self, text=None):
        if not text:
            return

        try:
            self.function = eval("lambda x: {}".format(text), self.globals)
        except SyntaxError as e:
            err = traceback.format_exception_only(type(e), e)
            QMessageBox.warning(None, "Error in expression",
                                "<pre>{1}{2}</pre>{3}".format(*err))
            return

        self.text = text
        self.valueChanged(None, self.value)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)

        if value is None:
            return

        self.value = value
        with SignalBlocker(self._internal_widget):
            try:
                text = "{}".format(self.function(value))
            except Exception as e:
                text = traceback.format_exception_only(type(e), e)[0]
            self._internal_widget.setText(text)
