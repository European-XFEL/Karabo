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

from karabo.common.scenemodel.api import EditableComboBoxModel
from karabo.common.states import State
from karabo.native import Configurable, Int32, String
from karabogui.binding.api import build_binding
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..combobox import EditableComboBox


class Object(Configurable):
    state = String(defaultValue=State.ON)
    prop = String(options=["foo", "bar", "baz", "qux"],
                  allowedStates=[State.ON])


class Other(Configurable):
    prop = Int32(options=[1, 2, 3, 5, 8])


@pytest.fixture
def combobox_setup(gui_app):
    # setup
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = EditableComboBox(proxy=proxy, model=EditableComboBoxModel())
    controller.create(None)
    assert controller.widget is not None
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_combobox_allowed(combobox_setup):
    controller, proxy = combobox_setup
    set_proxy_value(proxy, "state", "CHANGING")
    assert controller.widget.isEnabled() is False
    set_proxy_value(proxy, "state", "ON")
    assert controller.widget.isEnabled() is True


def test_combobox_set_value(combobox_setup):
    controller, proxy = combobox_setup
    set_proxy_value(proxy, "prop", "bar")
    assert controller.widget.currentIndex() == 1

    set_proxy_value(proxy, "prop", "nobar")
    assert controller.widget.currentIndex() == -1

    set_proxy_value(proxy, "prop", "baz")
    assert controller.widget.currentIndex() == 2


def test_combobox_edit_value(combobox_setup):
    controller, proxy = combobox_setup
    controller.widget.setCurrentIndex(3)
    assert proxy.edit_value == "qux"


def test_combobox_schema_update(gui_app):
    proxy = get_class_property_proxy(Other.getClassSchema(), "prop")
    controller = EditableComboBox(proxy=proxy)
    controller.create(None)
    assert controller.widget.currentIndex() == -1
    set_proxy_value(proxy, "prop", 2)
    assert controller.widget.currentIndex() == 1

    assert controller.widget.count() == 5

    build_binding(Object.getClassSchema(),
                  existing=proxy.root_proxy.binding)
    assert controller.widget.currentIndex() == -1
    assert controller.widget.count() == 4

    controller.destroy()


def test_combobox_focus(combobox_setup):
    controller, _ = combobox_setup
    combobox = controller.widget
    assert combobox.focusPolicy() == Qt.StrongFocus
