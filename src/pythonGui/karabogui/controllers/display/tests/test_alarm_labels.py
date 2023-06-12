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
from karabo.native import Configurable, Float, Int32
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.testing import set_proxy_value

from ..label import DisplayAlarmFloat, DisplayAlarmInteger


class Object(Configurable):
    number = Float(
        displayType="format|fmt=f&decimals=4",
        alarmLow=-2.0, alarmHigh=2.0,
        warnLow=-1.0, warnHigh=1.0)
    integer = Int32(
        alarmLow=-3, alarmHigh=3,
        warnLow=-1, warnHigh=1)


def test_alarm_numbers(gui_app):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    proxy = PropertyProxy(root_proxy=device, path="number")

    controller = DisplayAlarmFloat(proxy=proxy)
    # Initialize model for testing
    model = controller.model
    controller.initialize_model(proxy, model)
    controller.create(None)

    assert controller.fmt == "{:.4f}"
    assert model.warnLow == -1.0
    assert model.warnHigh == 1.0
    assert model.alarmHigh == 2.0
    assert model.alarmLow == -2.0

    set_proxy_value(proxy, "number", 0.75)
    assert controller.internal_widget.text() == "0.7500"
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(proxy, "number", 3.0)
    assert controller.bg_color == PROPERTY_ALARM_COLOR
    set_proxy_value(proxy, "number", 0.75)
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(proxy, "number", 1.2)
    assert controller.bg_color == PROPERTY_WARN_COLOR

    proxy = PropertyProxy(root_proxy=device, path="integer")
    controller = DisplayAlarmInteger(proxy=proxy)
    # Initialize model for testing
    model = controller.model
    controller.initialize_model(proxy, model)
    controller.create(None)

    assert model.warnLow == -1
    assert model.warnHigh == 1
    assert model.alarmHigh == 3
    assert model.alarmLow == -3

    set_proxy_value(proxy, "integer", 1)
    assert controller.internal_widget.text() == "1"
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(proxy, "integer", 4)
    assert controller.bg_color == PROPERTY_ALARM_COLOR
    set_proxy_value(proxy, "integer", 0)
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(proxy, "integer", 2)
    assert controller.bg_color == PROPERTY_WARN_COLOR
