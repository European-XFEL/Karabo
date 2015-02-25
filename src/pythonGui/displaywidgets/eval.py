__all__ = ["Evaluator"]


from util import SignalBlocker
from widget import DisplayWidget

from karabo.hashtypes import String, Simple

from PyQt4.QtGui import QLineEdit, QInputDialog, QAction


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
        action = QAction("Change expression...", self.widget)
        self.widget.addAction(action)
        action.triggered.connect(self.onChangeExpression)
        self.text = "x"
        self.value = None


    def function(self, x):
        """ this is just a placeholder """
        return x


    def onChangeExpression(self):
        text, ok = QInputDialog.getText(self.widget, "Enter Expression",
                                        "f(x) = ", text=self.text)
        if ok:
            self.setText(text)


    def setText(self, text):
        self.function = eval("lambda x:" + text, self.globals)
        self.text = text
        self.valueChanged(None, self.value)


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return

        self.value = value
        with SignalBlocker(self.widget):
            self.widget.setText("{}".format(self.function(value)))


    def save(self, e):
        e.set("expression", self.text)


    def load(self, e):
        self.setText(e.get("expression"))
