#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on July 9, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
    QAction, QFrame, QHBoxLayout, QInputDialog, QLabel, QSlider, QWidget)
from traits.api import Bool, Instance, on_trait_change

from karabo.common.api import (
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC)
from karabo.common.scenemodel.api import TickSliderModel
from karabogui.binding.api import (
    FloatBinding, IntBinding, get_editor_value, get_min_max)
from karabogui import messagebox
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import SignalBlocker

# Define a maximum range which can be represented by ticks
REASONABLE_RANGE = 100000
TICK_RANGE = 1000
WIDGET_HEIGHT = 20
WIDGET_WIDTH = 60


@register_binding_controller(ui_name='Tick Slider', can_edit=True,
                             klassname='TickSlider',
                             binding_type=(FloatBinding, IntBinding))
class TickSlider(BaseBindingController):
    # The scene model class for this controller
    model = Instance(TickSliderModel, args=())
    _error_shown = Bool(False)

    slider = Instance(QSlider)
    label = Instance(QLabel)

    def create_widget(self, parent):
        widget = QWidget(parent)
        layout = QHBoxLayout(widget)
        widget.setLayout(layout)

        self.slider = QSlider(Qt.Horizontal, widget)
        self.slider.setTickPosition(QSlider.TicksBelow)
        self.slider.setFocusPolicy(Qt.StrongFocus)
        self.slider.valueChanged.connect(self._edit_value)
        self.slider.setTickInterval(self.model.ticks)
        self.slider.setSingleStep(self.model.ticks)

        layout.addWidget(self.slider)

        self.label = QLabel(widget)
        self.label.setFrameStyle(QFrame.Box)
        self.label.setAlignment(Qt.AlignCenter)
        self.label.setVisible(self.model.show_value)
        self.label.setFixedSize(WIDGET_WIDTH, WIDGET_HEIGHT)
        self.label.setStyleSheet("font: 6pt; font-weight: bold;")
        layout.addWidget(self.label)

        action_ticks = QAction("Tick Interval", widget)
        action_ticks.triggered.connect(self.configure_tick_interval)
        widget.addAction(action_ticks)

        action_show_value = QAction("Show value", widget, checkable=True)
        action_show_value.toggled.connect(self.toggle_show_value)
        action_show_value.setChecked(self.model.show_value)
        widget.addAction(action_show_value)

        return widget

    def binding_update(self, proxy):
        attrs = proxy.binding.attributes
        min_inc = attrs.get(KARABO_SCHEMA_MIN_INC)
        min_exc = attrs.get(KARABO_SCHEMA_MIN_EXC)
        max_inc = attrs.get(KARABO_SCHEMA_MAX_INC)
        max_exc = attrs.get(KARABO_SCHEMA_MAX_EXC)
        low, high = get_min_max(proxy.binding)
        if (min_inc is None and min_exc is None or
                max_inc is None and max_exc is None or
                high - low > REASONABLE_RANGE):
            self._error_msg(proxy.path)
            self.slider.setEnabled(False)
            return

        with SignalBlocker(self.slider):
            if high - low > TICK_RANGE:
                self.slider.setTickPosition(QSlider.NoTicks)
            self.slider.setRange(low, high)

    def value_update(self, proxy):
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self.slider):
                self.slider.setValue(int(value))
                if isinstance(proxy.binding, IntBinding):
                    fmt = "{}"
                elif isinstance(proxy.binding, FloatBinding):
                    fmt = "{:.1f}"
                self.label.setText(fmt.format(value))

    def _error_msg(self, keyname):
        if self._error_shown:
            return
        self._error_shown = True
        msg = ('The value limits for {} are not set or too large,'
               ' please check the property attributes'.format(keyname))
        messagebox.show_warning(msg, title='No proper value limits',
                                parent=self.widget)

    @on_trait_change('model:show_value')
    def _show_value_update(self, value):
        self.label.setVisible(value)

    # ----------------------------------------------------------------------
    # Qt Slots

    # @pyqtSlot(object)
    def _edit_value(self, value):
        if self.proxy.binding is None or self._error_shown:
            return
        self.proxy.edit_value = value
        if value is not None:
            if isinstance(self.proxy.binding, IntBinding):
                fmt = "{}"
            elif isinstance(self.proxy.binding, FloatBinding):
                fmt = "{:.1f}"
            self.label.setText(fmt.format(value))

    # @pyqtSlot(bool)
    def toggle_show_value(self, value):
        self.model.show_value = value

    # @pyqtSlot()
    def configure_tick_interval(self, checked):
        """Configure the tick interval for this widget"""
        ticks, ok = QInputDialog.getInt(self.widget, 'Tick Interval',
                                        'Tick Interval:', self.model.ticks, 1,
                                        TICK_RANGE)
        if ok:
            self.model.ticks = ticks
            self.slider.setTickInterval(ticks)
            self.slider.setSingleStep(ticks)
