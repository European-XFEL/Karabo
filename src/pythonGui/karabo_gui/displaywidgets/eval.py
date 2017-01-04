
import traceback
from PyQt4.QtGui import QAction, QInputDialog, QLineEdit, QMessageBox

from karabo.middlelayer import String, Simple
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget


class Evaluator(DisplayWidget):
    category = String, Simple
    alias = "Evaluate Expression"

    globals = { }
    exec("from numpy import *", globals)

    def __init__(self, box, parent):
        super(Evaluator, self).__init__(box)

        self.widget = QLineEdit(parent)
        self.widget.setMinimumSize(160, 24)
        self.widget.setReadOnly(True)
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
        if text is None:
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
        if value is None:
            return

        self.value = value
        with SignalBlocker(self.widget):
            try:
                text = "{}".format(self.function(value))
            except Exception as e:
                text = traceback.format_exception_only(type(e), e)[0]
            self.widget.setText(text)


    def save(self, e):
        e.set("expression", self.text)


    def load(self, e):
        self.setText(e.get("expression"))
