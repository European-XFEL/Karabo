#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QDial, QSlider
from traits.api import Instance, Bool

from karabo.common.api import (
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC)
from karabo.common.scenemodel.api import KnobModel, SliderModel
from karabogui.binding.api import (
    FloatBinding, IntBinding, get_editor_value, get_min_max,
)
from karabogui import messagebox
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import SignalBlocker

# Define a maximum range which can be represented by the slider/knob
REASONABLE_RANGE = 1000


class _AnalogEditorWidget(BaseBindingController):
    _error_shown = Bool(False)

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
            self.widget.setEnabled(False)
        else:
            with SignalBlocker(self.widget):
                self.widget.setRange(low, high)

    def value_update(self, proxy):
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self.widget):
                self.widget.setValue(value)

    @pyqtSlot(object)
    def _edit_value(self, value):
        if self.proxy.binding is None or self._error_shown:
            return
        self.proxy.edit_value = value

    def _error_msg(self, keyname):
        # make sure you bother the user only once per widget
        if self._error_shown:
            return
        self._error_shown = True
        msg = ('Value limits for {} is not set or too large\n'
               'for this type of widget, please check the\n'
               ' property attributes'.format(keyname))
        messagebox.show_warning(msg, title='No proper value limit')


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
