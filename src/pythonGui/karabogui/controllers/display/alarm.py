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
from pathlib import Path

from qtpy.QtSvg import QSvgWidget
from traits.api import Instance

from karabo.common.scenemodel.api import GlobalAlarmModel
from karabogui import icons
from karabogui.binding.api import StringBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.indicators import get_alarm_svg

ICON_PATH = Path(icons.__file__).parent
SVG_OFFLINE = str(ICON_PATH.joinpath("statusOffline.svg"))


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
        # XXX: Only for backward compatibility
        return False

    def clear_widget(self):
        self.widget.load(SVG_OFFLINE)

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        svg = get_alarm_svg(value)
        if svg is None:
            svg = SVG_OFFLINE
        self.widget.load(svg)
