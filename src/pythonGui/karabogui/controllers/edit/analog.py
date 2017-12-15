#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QDial, QSlider
from traits.api import Instance

from karabo.common.scenemodel.api import KnobModel, SliderModel
from karabogui.binding.api import (
    FloatBinding, IntBinding, get_editor_value, get_min_max,
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC
)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import SignalBlocker


class _AnalogEditorWidget(BaseBindingController):

    def binding_update(self, proxy):
        attrs = proxy.binding.attributes
        min_inc = attrs.get(KARABO_SCHEMA_MIN_INC)
        min_exc = attrs.get(KARABO_SCHEMA_MIN_EXC)
        max_inc = attrs.get(KARABO_SCHEMA_MAX_INC)
        max_exc = attrs.get(KARABO_SCHEMA_MAX_EXC)
        low, high = get_min_max(proxy.binding)
        if min_inc is None and min_exc is None:
            low = 0
        if max_inc is None and max_exc is None:
            high = 100
        with SignalBlocker(self.widget):
            self.widget.setRange(low, high)

    def value_update(self, proxy):
        with SignalBlocker(self.widget):
            self.widget.setValue(get_editor_value(proxy))

    @pyqtSlot(object)
    def _edit_value(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value


@register_binding_controller(ui_name='Knob', can_edit=True, klassname='Knob',
                             binding_type=(FloatBinding, IntBinding))
class Knob(_AnalogEditorWidget):
    # The scene model class for this controller
    model = Instance(KnobModel, args=())

    def create_widget(self, parent):
        dial = QDial(parent)
        dial.setNotchesVisible(True)
        dial.setFocusPolicy(Qt.StrongFocus)
        dial.valueChanged.connect(self._edit_value)
        return dial


@register_binding_controller(ui_name='Slider', can_edit=True,
                             klassname='Slider',
                             binding_type=(FloatBinding, IntBinding))
class Slider(_AnalogEditorWidget):
    # The scene model class for this controller
    model = Instance(SliderModel, args=())

    def create_widget(self, parent):
        slider = QSlider(Qt.Horizontal, parent)
        slider.setTickPosition(QSlider.TicksBelow)
        slider.setFocusPolicy(Qt.StrongFocus)
        slider.valueChanged.connect(self._edit_value)
        return slider
