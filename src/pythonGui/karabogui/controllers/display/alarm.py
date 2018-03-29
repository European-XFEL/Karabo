#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 28, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance

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

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if self.widget is None or value is None:
            return

        svg = get_alarm_svg(value)
        self.widget.load(svg)
