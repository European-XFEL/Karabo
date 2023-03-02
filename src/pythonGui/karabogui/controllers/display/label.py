#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from qtpy.QtWidgets import QAction, QDialog
from traits.api import Instance, Undefined

from karabo.common.api import (
    KARABO_ALARM_HIGH, KARABO_ALARM_LOW, KARABO_WARN_HIGH, KARABO_WARN_LOW)
from karabo.common.scenemodel.api import (
    DisplayAlarmFloatModel, DisplayFloatModel, DisplayLabelModel,
    build_model_config)
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, IntBinding, StringBinding,
    get_dtype_format)
from karabogui.controllers.api import (
    BaseFloatController, BaseLabelController, register_binding_controller)
from karabogui.dialogs.api import AlarmDialog
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)

BINDING_TYPES = (StringBinding, CharBinding, ComplexBinding, IntBinding,
                 FloatBinding)


@register_binding_controller(ui_name="Value Field",
                             klassname="DisplayLabel",
                             binding_type=BINDING_TYPES, priority=20)
class DisplayLabel(BaseLabelController):
    model = Instance(DisplayLabelModel, args=())

    def binding_update_proxy(self, proxy):
        self.fmt = get_dtype_format(proxy.binding)

    def value_update_proxy(self, proxy, value):
        attributes = proxy.binding.attributes
        alarm_low = attributes.get(KARABO_ALARM_LOW)
        alarm_high = attributes.get(KARABO_ALARM_HIGH)
        warn_low = attributes.get(KARABO_WARN_LOW)
        warn_high = attributes.get(KARABO_WARN_HIGH)
        if ((alarm_low is not None and value < alarm_low) or
                (alarm_high is not None and value > alarm_high)):
            self.bg_color = PROPERTY_ALARM_COLOR
        elif ((warn_low is not None and value < warn_low) or
              (warn_high is not None and value > warn_high)):
            self.bg_color = PROPERTY_WARN_COLOR
        else:
            self.bg_color = ALL_OK_COLOR
        sheet = self.style_sheet.format(self.bg_color)
        self.widget.setStyleSheet(sheet)


@register_binding_controller(ui_name="Float Field",
                             klassname="DisplayFloat",
                             binding_type=(FloatBinding, ComplexBinding),
                             priority=10)
class DisplayFloat(BaseFloatController):
    model = Instance(DisplayFloatModel, args=())


@register_binding_controller(ui_name="Alarm Float Field",
                             klassname="DisplayAlarmFloat",
                             binding_type=(FloatBinding, ComplexBinding),
                             priority=0)
class DisplayAlarmFloat(BaseFloatController):
    model = Instance(DisplayAlarmFloatModel, args=())

    def create_widget(self, parent):
        widget = super().create_widget(parent)
        alarm_action = QAction("Configure alarms", widget)
        alarm_action.triggered.connect(self._alarm_dialog)
        widget.addAction(alarm_action)

        return widget

    @staticmethod
    def initialize_model(proxy, model):
        """Initialize the formatting from the binding of the proxy"""
        super(DisplayAlarmFloat, DisplayAlarmFloat).initialize_model(
            proxy, model)
        attributes = proxy.binding.attributes
        traits = {}
        for key in [KARABO_ALARM_LOW, KARABO_WARN_LOW,
                    KARABO_WARN_HIGH, KARABO_ALARM_HIGH]:
            traits[key] = attributes.get(key, Undefined)
        model.trait_set(**traits)

    def value_update_proxy(self, proxy, value):
        model = self.model
        alarm_low = model.alarmLow
        alarm_high = model.alarmHigh
        warn_low = model.warnLow
        warn_high = model.warnHigh
        if ((alarm_low is not Undefined and value < alarm_low) or
                (alarm_high is not Undefined and value > alarm_high)):
            self.bg_color = PROPERTY_ALARM_COLOR
        elif ((warn_low is not Undefined and value < warn_low) or
              (warn_high is not Undefined and value > warn_high)):
            self.bg_color = PROPERTY_WARN_COLOR
        else:
            self.bg_color = ALL_OK_COLOR
        sheet = self.style_sheet.format(self.bg_color)
        self.widget.setStyleSheet(sheet)

    def _alarm_dialog(self):
        binding = self.proxy.binding
        config = build_model_config(self.model)
        dialog = AlarmDialog(binding, config, parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(**dialog.values)
            self.value_update(self.proxy)
