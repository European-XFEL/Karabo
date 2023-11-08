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
from qtpy.QtWidgets import QWidget

from karabo.common.scenemodel.api import GlobalAlarmModel
from karabo.native import Configurable, String
from karabogui.alarms.api import get_alarm_svg
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..alarm import DisplayAlarm


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class AlarmObject(Configurable):
    alarmCondition = String(
        defaultValue="alarm",
        displayType="AlarmCondition")

    noAlarmCondition = String(
        defaultValue="alarm")

    anotherAlarmCondition = String(
        defaultValue="alarm",
        displayType="AlarmCondition")


schema = AlarmObject.getClassSchema()
new_schema = AlarmObject.getClassSchema()


def test_alarm_controller(gui_app, mocker):
    proxy = get_class_property_proxy(schema, "alarmCondition")
    new_proxy = get_class_property_proxy(new_schema, "alarmCondition")
    model = GlobalAlarmModel()

    target = "karabogui.controllers.display.alarm.QSvgWidget"

    # Patch to fake Svg behavior
    mocker.patch(target, new=MockQSvgWidget)

    proxy.value = "warn"
    controller = DisplayAlarm(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None

    for alarm_type in ("warn", "interlock", "alarm", "none"):
        set_proxy_value(proxy, "alarmCondition", alarm_type)
        active = get_alarm_svg(alarm_type)
        assert controller.widget.loaded_data == active

    none_svg = get_alarm_svg("none")
    assert controller.widget.loaded_data == none_svg

    set_proxy_value(new_proxy, "alarmCondition", "warn")
    # Add a new proxy
    controller.add_proxy(new_proxy)
    warn_svg = get_alarm_svg("warn")
    assert controller.widget.loaded_data == warn_svg

    # Change the old ``none`` alarm to ``alarm``
    set_proxy_value(proxy, "alarmCondition", "alarm")
    alarm_svg = get_alarm_svg("alarm")
    assert controller.widget.loaded_data == alarm_svg

    no_alarm_proxy = get_class_property_proxy(new_schema, "noAlarmCondition")
    assert no_alarm_proxy.binding is not None
    assert not controller.visualize_additional_property(no_alarm_proxy)

    another_alarm_proxy = get_class_property_proxy(
        new_schema, "anotherAlarmCondition")
    assert another_alarm_proxy.binding is not None
    assert controller.visualize_additional_property(another_alarm_proxy)

    controller.destroy()
    assert controller.widget is None
