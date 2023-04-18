#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

from traits.api import Instance

from karabo.common.api import (
    KARABO_ALARM_HIGH, KARABO_ALARM_LOW, KARABO_WARN_HIGH, KARABO_WARN_LOW)
from karabo.common.scenemodel.api import (
    DisplayAlarmFloatModel, DisplayAlarmIntegerModel, DisplayFloatModel,
    DisplayLabelModel)
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, IntBinding, StringBinding,
    get_dtype_format)
from karabogui.controllers.api import (
    AlarmMixin, BaseLabelController, FormatMixin, register_binding_controller)
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
class DisplayFloat(FormatMixin, BaseLabelController):
    model = Instance(DisplayFloatModel, args=())


@register_binding_controller(ui_name="Alarm Float Field",
                             klassname="DisplayAlarmFloat",
                             binding_type=(FloatBinding, ComplexBinding),
                             priority=0)
class DisplayAlarmFloat(AlarmMixin, FormatMixin, BaseLabelController):
    model = Instance(DisplayAlarmFloatModel, args=())


@register_binding_controller(ui_name="Alarm Integer Field",
                             klassname="DisplayAlarmInteger",
                             binding_type=IntBinding,
                             priority=0)
class DisplayAlarmInteger(AlarmMixin, BaseLabelController):
    model = Instance(DisplayAlarmIntegerModel, args=())
