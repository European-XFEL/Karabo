#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 28, 2018
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################

from qtpy.QtSvg import QSvgWidget
from traits.api import Instance

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.scenemodel.api import GlobalAlarmModel
from karabogui.binding.api import StringBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.indicators import get_alarm_svg


@register_binding_controller(ui_name="Alarm Widget",
                             klassname="DisplayAlarm",
                             is_compatible=with_display_type("AlarmCondition"),
                             priority=90,
                             binding_type=StringBinding)
class DisplayAlarm(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(GlobalAlarmModel, args=())

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        return widget

    def add_proxy(self, proxy):
        """Add an alarm condition proxy to the widget"""
        if proxy.binding is None:
            return True
        if not proxy.binding.display_type == "AlarmCondition":
            return False

        self._update_alarm_widget(proxy)
        return True

    def _update_alarm_widget(self, proxy=None):
        widget_alarms = [AlarmCondition(get_binding_value(p, "none"))
                         for p in self.proxies]
        if proxy is not None:
            widget_alarms.append(
                AlarmCondition(get_binding_value(proxy, "none")))
        widget_alarms.sort()
        alarm_type = widget_alarms[-1].asString()
        svg = get_alarm_svg(alarm_type)
        self.widget.load(svg)

    def value_update(self, proxy):
        self._update_alarm_widget()
