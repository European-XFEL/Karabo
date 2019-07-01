import operator
import os

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QDialog, QSizePolicy, QSlider, QVBoxLayout

from karabogui.util import SignalBlocker


class LevelsDialog(QDialog):

    def __init__(self, levels, image_range, auto_levels, parent=None):
        super(LevelsDialog, self).__init__(parent)
        ui_path = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                               'levels_dialog.ui')
        uic.loadUi(ui_path, self)

        # Check if autolevel: image levels and range are almost equal.
        # This is with a tolerance of 1%.
        self.automatic_checkbox.setChecked(auto_levels)
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
        self.values_widget.setEnabled(not auto_levels)

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
        default = image_range if auto_levels else levels
        self._set_default_values(default)

    def _set_minmax_values(self, levels):
        # Set widget min and max values
        min_level, max_level = levels

        for widget in self._min_widgets:
            widget.setMinimum(min_level)
            widget.setMaximum(max_level)
        for widget in self._max_widgets:
            widget.setMinimum(min_level)
            widget.setMaximum(max_level)

        self.min_label.setText(str(min_level))
        self.max_label.setText(str(max_level))
        self.max_slider.setTickInterval(max_level)

    def _set_default_values(self, levels):
        min_level, max_level = levels

        for widget in self._min_widgets:
            with SignalBlocker(widget):
                widget.setValue(min_level)
        for widget in self._max_widgets:
            with SignalBlocker(widget):
                widget.setValue(max_level)

        self.min_slider.setRestrictedValue(max_level)
        self.max_slider.setRestrictedValue(min_level)

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
        self._type = type_
        self._restricted_value = None
        self.valueChanged.connect(self.restrictMove)

        self.setSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.Fixed)
        self.setMinimumWidth(240)

        self._compare = operator.gt if type_ == FloatSlider.Minimum \
            else operator.lt

    def value(self):
        return float(super(FloatSlider, self).value()) / self._multi

    def minimum(self):
        return super(FloatSlider, self).minimum() / self._multi

    def maximum(self):
        return super(FloatSlider, self).maximum() / self._multi

    def setMinimum(self, value):
        super(FloatSlider, self).setMinimum(value * self._multi)
        if (self._restricted_value is None
                and self._type == FloatSlider.Maximum):
            self._restricted_value = value

    def setMaximum(self, value):
        super(FloatSlider, self).setMaximum(value * self._multi)
        if (self._restricted_value is None
                and self._type == FloatSlider.Minimum):
            self._restricted_value = value

    def setValue(self, value):
        super(FloatSlider, self).setValue(int(value * self._multi))

    def setRestrictedValue(self, value):
        self._restricted_value = value

    def setTickInterval(self, value):
        super(FloatSlider, self).setTickInterval(int(value * self._multi))

    @pyqtSlot()
    def restrictMove(self):
        if (self._restricted_value is not None and
                self._compare(self.value(), self._restricted_value)):
            self.setSliderPosition(int(self._restricted_value * self._multi))
