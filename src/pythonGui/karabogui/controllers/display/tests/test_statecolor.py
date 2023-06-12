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

from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayStateColorModel
from karabo.native import Configurable, String
from karabogui.controllers.util import get_class_const_trait
from karabogui.indicators import STATE_COLORS
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..statecolor import DisplayStateColor


class Object(Configurable):
    state = String()


@pytest.fixture
def statecolor_setup(gui_app):
    # setup
    schema = Object.getClassSchema()
    model = DisplayStateColorModel()
    proxy = get_class_property_proxy(schema, "state")
    controller = DisplayStateColor(model=model, proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    yield controller, proxy

    # teardown
    controller.destroy()
    assert controller.widget is None


def test_statecolor_set_values(statecolor_setup):
    controller, proxy = statecolor_setup

    states = ("CHANGING", "ACTIVE", "PASSIVE", "DISABLED", "RUNNING",
              "STATIC", "NORMAL", "ERROR", "INIT", "UNKNOWN")
    for state in states:
        set_proxy_value(proxy, "state", state)
        color = STATE_COLORS[getattr(State, state)]
        sheet = controller.widget.styleSheet()
        assert str(color) in sheet


def test_statecolor_string(statecolor_setup):
    controller, proxy = statecolor_setup
    action = controller.widget.actions()[0]
    assert action.text() == "Show State String"
    state = "CHANGING"
    set_proxy_value(proxy, "state", state)
    assert controller.widget.text() == ""
    action.trigger()
    assert controller.widget.text() == "CHANGING"
    state = "ON"
    set_proxy_value(proxy, "state", state)
    assert controller.widget.text() == "ON"
    action.trigger()
    assert controller.widget.text() == ""


def test_statecolor_priority(statecolor_setup):
    controller, _ = statecolor_setup
    assert get_class_const_trait(controller, "_priority") == 90
