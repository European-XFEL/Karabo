import operator
import os

from PyQt4 import uic
from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt4.QtGui import QDialog, QSizePolicy, QSlider, QVBoxLayout

from karabogui.util import SignalBlocker

from ..utils import levels_almost_equal


class LevelsDialog(QDialog):
    set_image_levels_signal = pyqtSignal(object)

    def __init__(self, levels, image_range, parent=None):
        super(LevelsDialog, self).__init__(parent)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'levels_dialog.ui')
        uic.loadUi(ui_path, self)

        # Check if autolevel: image levels and range are almost equal.
        # This is with a tolerance of 1%.
        autolevel = levels_almost_equal(levels, image_range)
        self.automatic_checkbox.setChecked(autolevel)
        self.automatic_checkbox.stateChanged.connect(self.set_automatic_levels)

        self.min_slider = FloatSlider(FloatSlider.Minimum)
        min_vbox = QVBoxLayout()
        min_vbox.addWidget(self.min_slider)
        self.min_slider_widget.setLayout(min_vbox)

        self.max_slider = FloatSlider(FloatSlider.Maximum)
        max_vbox = QVBoxLayout()
        max_vbox.addWidget(self.max_slider)
        self.max_slider_widget.setLayout(max_vbox)
        self.max_slider.setTickPosition(QSlider.TicksBelow)
        self.values_widget.setEnabled(not autolevel)

        self.min_slider.valueChanged.connect(self._change_min_by_slider)
        self.min_spinbox.valueChanged.connect(self._change_min_by_spinbox)
        self.max_slider.valueChanged.connect(self.change_max_by_slider)
        self.max_spinbox.valueChanged.connect(self._change_max_by_field)
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self._min_widgets = (self.min_spinbox, self.min_slider)
        self._max_widgets = (self.max_spinbox, self.max_slider)

        self._set_minmax_values(image_range)

        # Set widget default values
        default = image_range if autolevel else levels
        self._set_default_values(default)

    def _set_minmax_values(self, levels):
        # Set widget min and max values
        _min, _max = levels

        for widget in self._min_widgets:
            widget.setMinimumWidth(_min)
            widget.setMaximum(_max)
        for widget in self._max_widgets:
            widget.setMinimumWidth(_min)
            widget.setMaximum(_max)

        self.min_label.setText(str(_min))
        self.max_label.setText(str(_max))
        self.max_slider.setTickInterval(_max)

    def _set_default_values(self, levels):
        _min, _max = levels

        for widget in self._min_widgets:
            with SignalBlocker(widget):
                widget.setValue(_min)
        for widget in self._max_widgets:
            with SignalBlocker(widget):
                widget.setValue(_max)

        self.min_slider.setRestrictedValue(_max)
        self.max_slider.setRestrictedValue(_min)

    @pyqtSlot()
    def _change_min_by_slider(self):
        value = self.min_slider.value()
        if value == self.min_spinbox.value():
            return

        with SignalBlocker(self.min_spinbox):
            self.min_spinbox.setValue(value)

        self._min_level_changed(value)

    @pyqtSlot(float)
    def _change_min_by_spinbox(self, value):
        with SignalBlocker(self.min_slider):
            self.min_slider.setValue(value)

        self._min_level_changed(value)

    @pyqtSlot()
    def change_max_by_slider(self):
        value = self.max_slider.value()
        if value == self.max_spinbox.value():
            return

        with SignalBlocker(self.max_spinbox):
            self.max_spinbox.setValue(value)

        self._max_level_changed(value)

    @pyqtSlot(float)
    def _change_max_by_field(self, value):
        with SignalBlocker(self.max_slider):
            self.max_slider.setValue(value)

        self._max_level_changed(value)

    def _min_level_changed(self, value):
        self.max_slider.setRestrictedValue(value)
        self.max_spinbox.setMinimum(value)

    def _max_level_changed(self, value):
        self.min_slider.setRestrictedValue(value)
        self.min_spinbox.setMaximum(value)

    @pyqtSlot(object)
    def set_automatic_levels(self, state):
        self.values_widget.setEnabled(state == Qt.Unchecked)
        self.min_spinbox.setValue(float(self.min_label.text()))
        self.max_spinbox.setValue(float(self.max_label.text()))

    @property
    def levels(self):
        levels = None if self.automatic_checkbox.isChecked() else \
            [self.min_spinbox.value(), self.max_spinbox.value()]
        return levels


class FloatSlider(QSlider):
    Minimum = 0
    Maximum = 1

    def __init__(self, type_, orientation=Qt.Horizontal, decimals=2,
                 parent=None):
        super(FloatSlider, self).__init__(orientation, parent)
        self._multi = 10 ** decimals
        self.setMinimum(self.minimum())
        self.setMaximum(self.maximum())
        self._restricted_value = self.minimum()
        self.valueChanged.connect(self.restrictMove)

        self.setSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.Fixed)
        self.setMinimumWidth(240)

        self._compare = operator.gt if type_ == FloatSlider.Minimum \
            else operator.lt

    def value(self):
        return float(super(FloatSlider, self).value()) / self._multi

    def setMinimum(self, value):
        return super(FloatSlider, self).setMinimum(value * self._multi)

    def setMaximum(self, value):
        return super(FloatSlider, self).setMaximum(value * self._multi)

    def setValue(self, value):
        super(FloatSlider, self).setValue(int(value * self._multi))

    def setRestrictedValue(self, value):
        self._restricted_value = value

    def setTickInterval(self, value):
        super(FloatSlider, self).setTickInterval(int(value * self._multi))

    @pyqtSlot()
    def restrictMove(self):
        if self._compare(self.value(), self._restricted_value):
            self.setSliderPosition(int(self._restricted_value * self._multi))
