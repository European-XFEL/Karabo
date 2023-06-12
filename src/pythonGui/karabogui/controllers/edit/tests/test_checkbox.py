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
from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import CheckBoxModel
from karabo.common.states import State
from karabo.native import Bool, Configurable, String
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..checkbox import EditableCheckBox


class Object(Configurable):
    state = String(defaultValue=State.ON)
    prop = Bool(allowedStates=[State.ON])


@pytest.fixture
def checkbox_setup(gui_app):
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = EditableCheckBox(proxy=proxy, model=CheckBoxModel())
    controller.create(None)
    assert controller.widget is not None

    yield controller, proxy

    controller.destroy()
    assert controller.widget is None


def test_checkbox_state_update(checkbox_setup):
    controller, proxy = checkbox_setup
    set_proxy_value(proxy, "state", "CHANGING")
    assert controller.widget.isEnabled() is False
    set_proxy_value(proxy, "state", "ON")
    assert controller.widget.isEnabled() is True


def test_checkbox_set_value(checkbox_setup):
    controller, proxy = checkbox_setup
    set_proxy_value(proxy, "prop", True)
    assert controller.widget.checkState() == Qt.Checked


def test_checkbox_edit_value(checkbox_setup):
    controller, proxy = checkbox_setup
    controller.widget.setCheckState(Qt.Checked)
    assert proxy.edit_value

    controller.widget.setCheckState(Qt.Unchecked)
    assert not proxy.edit_value
