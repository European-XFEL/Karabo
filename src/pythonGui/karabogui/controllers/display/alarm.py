#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 28, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.scenemodel.api import GlobalAlarmModel
from karabogui.alarms.api import get_alarm_svg

from karabogui.binding.api import get_binding_value, StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)


@register_binding_controller(ui_name='Alarm Widget',
                             klassname='DisplayAlarm',
                             is_compatible=with_display_type('AlarmCondition'),
                             binding_type=StringBinding)
class DisplayAlarm(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(GlobalAlarmModel, args=())

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        return widget

    def add_proxy(self, proxy):
        """Add an alarm condition proxy to the widget"""
        widget_alarms = [AlarmCondition(get_binding_value(p, "none"))
                         for p in self.proxies if p]
        widget_alarms.append(AlarmCondition(get_binding_value(proxy, "none")))
        widget_alarms.sort()
        alarm_type = widget_alarms[-1].asString()
        svg = get_alarm_svg(alarm_type)
        self.widget.load(svg)
        return True

    def value_update(self, proxy):
        widget_alarms = [AlarmCondition(get_binding_value(p, "none"))
                         for p in self.proxies if p]
        widget_alarms.sort()
        alarm_type = widget_alarms[-1].asString()
        svg = get_alarm_svg(alarm_type)
        self.widget.load(svg)
