from numpy import log10
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QDoubleSpinBox, QInputDialog

from karabo.middlelayer import Number
from karabo_gui.displaywidgets.unitlabel import add_unit_label
from karabo_gui.const import WIDGET_MIN_HEIGHT, WIDGET_MIN_WIDTH
from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget, EditableWidget


class FloatSpinBox(EditableWidget, DisplayWidget):
    category = Number
    alias = "SpinBox (real)"

    def __init__(self, box, parent):
        super().__init__(box)
        self._internal_widget = QDoubleSpinBox(parent)
        self._internal_widget.setMinimumSize(WIDGET_MIN_WIDTH,
                                             WIDGET_MIN_HEIGHT)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)

        # add actions
        step_action = QAction("Change Step...", self)
        step_action.triggered.connect(self.change_step)
        self.widget.addAction(step_action)
        decimal_action = QAction("Change Decimals...", self)
        decimal_action.triggered.connect(self.change_decimals)
        self.widget.addAction(decimal_action)

        self.decimals = None

    @property
    def editWidget(self):
        return self._internal_widget

    @pyqtSlot()
    def change_step(self):
        step, ok = QInputDialog.getDouble(
            self.widget, "Single Step", "Enter size of a single step",
            self._internal_widget.singleStep())
        if ok:
            self._set_step(step)

    @pyqtSlot()
    def change_decimals(self):
        decimals, ok = QInputDialog.getInt(
            self.widget, "Decimals", "Enter number of decimals",
            value=self.decimals, min=0, max=15)
        if ok:
            self._set_decimals(decimals)

    def setReadOnly(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.valueChanged[float].connect(
                self.onValueChanged)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    @pyqtSlot(float)
    def onValueChanged(self, value):
        EditableWidget.onEditingFinished(self, value)

    def typeChanged(self, box):
        self._internal_widget.setRange(*box.descriptor.getMinMax())
        abs_error = box.descriptor.absoluteError
        if abs_error is not None and abs_error < 1:
            decimals = -log10(abs_error)
            self._set_decimals(decimals)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setValue(value)

    @property
    def value(self):
        return self._internal_widget.value()

    def _set_step(self, value):
        self._internal_widget.setSingleStep(value)

    def _set_decimals(self, value):
        self.decimals = value
        self._internal_widget.setDecimals(value)
