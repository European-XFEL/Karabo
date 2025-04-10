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
from unittest.mock import Mock

from qtpy.QtWidgets import QWidget
from traits.api import Instance

from karabo.common.scenemodel.bases import BaseEditWidget, BaseWidgetObjectData
from karabo.native import Configurable, Hash, Int8, Int32
from karabogui.binding.api import Int32Binding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.sceneview.widget.api import ControllerContainer
from karabogui.testing import get_property_proxy, singletons


class IntDevice(Configurable):
    base = Int32(
        defaultValue=0)
    rotary = Int32(
        defaultValue=0)
    linear = Int32(
        defaultValue=0)
    forbidden = Int8(
        defaultValue=0)


class DisplayModel(BaseWidgetObjectData):
    """A test display model for controllers"""


class EditableModel(BaseEditWidget):
    """A test editable model for controllers"""


@register_binding_controller(can_edit=False, klassname="TestDisplay",
                             binding_type=Int32Binding)
class DisplayController(BaseBindingController):
    model = Instance(DisplayModel, args=())

    def create_widget(self, parent):
        return QWidget(parent)

    def add_proxy(self, proxy):
        return True

    def remove_proxy(self, proxy):
        return False


@register_binding_controller(can_edit=False, klassname="TestEditable",
                             binding_type=Int32Binding)
class EditableController(BaseBindingController):
    model = Instance(EditableModel, args=())

    def create_widget(self, parent):
        return QWidget(parent)

    def add_proxy(self, proxy):
        return True

    def remove_proxy(self, proxy):
        return True


def test_display_widget_container(gui_app):
    """Test the controller container with display controller"""
    model = DisplayModel(keys=["TestDevice.base", "TestDevice.rotary"])
    container = ControllerContainer(DisplayController, model, None)

    controller = container.widget_controller
    assert controller.widget is not None
    assert container.toolTip() == "TestDevice.base, TestDevice.rotary"
    proxy = get_property_proxy(IntDevice.getClassSchema(), "linear")

    assert container.add_proxies([proxy])
    assert len(controller.proxies) == 3
    tooltip = "TestDevice.base, TestDevice.rotary, TestDevice.linear"
    assert container.toolTip() == tooltip

    # We are a readonly container, nothing gets send outside
    network = Mock()
    manager = Mock()
    with singletons(network=network, manager=manager):
        container.apply_changes()
        network.onReconfigure.assert_not_called()
        # Apply when we are visible
        container.set_visible(True)
        container.apply_changes()
        network.onReconfigure.assert_not_called()
        proxy.edit_value = 2
        container.apply_changes()
        network.onReconfigure.assert_not_called()

    # Try to add a forbidden proxy
    forbidden = get_property_proxy(IntDevice.getClassSchema(), "forbidden")
    assert not container.add_proxies([forbidden])
    assert len(controller.proxies) == 3
    assert container.toolTip() == tooltip

    # Try to remove although controller does not allow
    assert not container.remove_proxies([proxy])
    assert len(controller.proxies) == 3
    assert container.toolTip() == tooltip
    container.destroy()


def test_editable_widget_container(gui_app):
    """Test the controller container with editable controller"""
    model = EditableModel(keys=["TestDevice.base", "TestDevice.rotary"])
    container = ControllerContainer(EditableController, model, None)

    controller = container.widget_controller
    assert controller.widget is not None

    assert container.toolTip() == "TestDevice.base, TestDevice.rotary"
    proxy = get_property_proxy(IntDevice.getClassSchema(), "linear")
    assert container.add_proxies([proxy])
    assert len(controller.proxies) == 3
    assert container.toolTip() == (
        "AccessLevel: OPERATOR - Access: True\n\nTestDevice.base, "
        "TestDevice.rotary, TestDevice.linear")
    network = Mock()
    manager = Mock()
    with singletons(network=network, manager=manager):
        # Reconfigure single proxy
        proxy.edit_value = 2
        container.apply_changes()
        network.onReconfigure.assert_not_called()
        container.set_visible(True)
        container.apply_changes()
        network.onReconfigure.assert_called_with("TestDevice",
                                                 Hash("linear", 2))
        network.reset_mock()
        # Reconfigure twice
        container.apply_changes()
        network.onReconfigure.assert_called_with("TestDevice",
                                                 Hash("linear", 2))
        network.reset_mock()
        # Decline changes
        assert proxy.edit_value is not None
        container.decline_changes()
        assert proxy.edit_value is None
        container.apply_changes()
        network.onReconfigure.assert_not_called()

    assert container.remove_proxies([proxy])
    assert len(controller.proxies) == 2
    container.destroy()
