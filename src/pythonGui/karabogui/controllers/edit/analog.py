#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QDial, QSlider
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import KnobModel, SliderModel
from karabogui.binding.api import (
    BaseBindingController, FloatBinding, IntBinding,
    register_binding_controller, get_min_max, KARABO_SCHEMA_MAX_EXC,
    KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_INC
)


class _AnalogEditorWidget(BaseBindingController):
    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        if self.widget is None:
            return

        attrs = binding.attributes
        min_inc = attrs.get(KARABO_SCHEMA_MIN_INC)
        min_exc = attrs.get(KARABO_SCHEMA_MIN_EXC)
        max_inc = attrs.get(KARABO_SCHEMA_MAX_INC)
        max_exc = attrs.get(KARABO_SCHEMA_MAX_EXC)
        low, high = get_min_max(binding)
        if min_inc is None and min_exc is None:
            low = 0
        if max_inc is None and max_exc is None:
            high = 100
        self.widget.setRange(low, high)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        if self.widget is not None:
            self.widget.setValue(value)

    @pyqtSlot(object)
    def _edit_value(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.value = value

    def _widget_changed(self):
        # Max sure a range gets set on the widget
        if self.proxy.binding is None:
            return
        self._binding_update(self.proxy.binding)


@register_binding_controller(ui_name='Knob',
                             binding_type=(FloatBinding, IntBinding))
class Knob(_AnalogEditorWidget):
    # The scene model class for this controller
    model = Instance(KnobModel)

    def create_widget(self, parent):
        dial = QDial(parent)
        dial.setNotchesVisible(True)
        dial.setFocusPolicy(Qt.StrongFocus)
        dial.valueChanged.connect(self._edit_value)
        return dial


@register_binding_controller(ui_name='Slider',
                             binding_type=(FloatBinding, IntBinding))
class Slider(_AnalogEditorWidget):
    # The scene model class for this controller
    model = Instance(SliderModel)

    def create_widget(self, parent):
        slider = QSlider(Qt.Horizontal, parent)
        slider.setTickPosition(QSlider.TicksBelow)
        slider.setFocusPolicy(Qt.StrongFocus)
        slider.valueChanged.connect(self._edit_value)
        return slider
