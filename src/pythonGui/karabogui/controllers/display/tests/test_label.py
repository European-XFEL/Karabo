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
import pytest

from karabo.native import Configurable, Float, String
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.indicators import ALL_OK_COLOR
from karabogui.testing import set_proxy_value

from ..label import DisplayLabel


class Object(Configurable):
    string = String()
    alarms = Float()
    faulty = Float(displayType="fmt|{:*1271f}")


@pytest.fixture
def label_setup(gui_app):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    string = PropertyProxy(root_proxy=device, path="string")
    alarms = PropertyProxy(root_proxy=device, path="alarms")
    faulty = PropertyProxy(root_proxy=device, path="faulty")
    yield string, alarms, faulty


def test_basics(label_setup):
    string, alarms, faulty = label_setup
    controller = DisplayLabel(proxy=string)
    controller.create(None)
    assert controller.widget is not None

    controller.destroy()
    assert controller.widget is None


def test_set_string_value(label_setup):
    string, alarms, faulty = label_setup
    controller = DisplayLabel(proxy=string)
    controller.create(None)
    set_proxy_value(string, "string", "hello")
    assert controller.internal_widget.text() == "hello"


def test_alarm_color(label_setup):
    string, alarms, faulty = label_setup
    controller = DisplayLabel(proxy=alarms)
    controller.create(None)
    set_proxy_value(alarms, "alarms", 0.75)
    assert controller.internal_widget.text() == "0.75"
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(alarms, "alarms", 3.0)
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(alarms, "alarms", 1.5)
    assert controller.bg_color == ALL_OK_COLOR


def test_wrong_format(label_setup):
    string, alarms, faulty = label_setup
    controller = DisplayLabel(proxy=faulty)
    controller.create(None)
    set_proxy_value(faulty, "faulty", 0.25)
    assert controller.internal_widget.text() == "0.25"
