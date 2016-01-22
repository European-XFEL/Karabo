__all__ = ["FloatSpinBox"]

from numpy import log10

from PyQt4.QtGui import QAction, QDoubleSpinBox, QInputDialog

from karabo.api_2 import Number

from karabo_gui.const import ns_karabo
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget, EditableWidget


class FloatSpinBox(EditableWidget, DisplayWidget):
    category = Number
    alias = "Spin Box"

    def __init__(self, box, parent):
        super().__init__(box)
        self.widget = QDoubleSpinBox(parent)
        action = QAction("Change Step...", self)
        action.triggered.connect(self.changeStep)
        self.widget.addAction(action)

    def changeStep(self):
        step, ok = QInputDialog.getDouble(
            self.widget, "Single Step", "Enter size of a single step",
            self.widget.singleStep())
        if ok:
            self.widget.setSingleStep(step)

    def setReadOnly(self, ro):
        self.widget.setReadOnly(ro)
        if not ro:
            self.widget.valueChanged[float].connect(self.onValueChanged)

    def onValueChanged(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)

    def typeChanged(self, box):
        self.widget.setRange(*box.descriptor.getMinMax())
        ae = box.descriptor.absoluteError
        if ae is not None and ae < 1:
            self.widget.setDecimals(-log10(ae))

    def valueChanged(self, box, value, timestamp=None):
        with SignalBlocker(self.widget):
            self.widget.setValue(value)

    @property
    def value(self):
        return self.widget.value()

    def save(self, element):
        element.set(ns_karabo + "step", repr(self.widget.singleStep()))

    def load(self, element):
        self.widget.setSingleStep(float(element.get(ns_karabo + "step")))
