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
# AccessSlottedDevice
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtWidgets import QDialog, QMessageBox, QToolButton

from karabo.common.states import State
from karabo.native import AccessLevel, Configurable, Slot, String
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, apply_default_configuration, build_binding)
from karabogui.const import IS_MAC_SYSTEM
from karabogui.testing import (
    access_level, click_button, set_proxy_value, singletons)
from karabogui.topology.system_tree import SystemTreeNode

from ..command import DisplayCommand


class EmptyDevice(Configurable):
    """What a device looks like to a scene when it is offline"""


class SlottedDevice(Configurable):
    state = String(defaultValue=State.INIT)

    @Slot(allowedStates=[State.INIT],
          displayedName="Call ME", requiredAccessLevel=AccessLevel.OBSERVER)
    def callme(self):
        pass

    @Slot(allowedStates=[State.ACTIVE],
          requiredAccessLevel=AccessLevel.OBSERVER)
    def yep(self):
        pass


class AccessSlottedDevice(Configurable):
    state = String(defaultValue=State.INIT)

    @Slot(allowedStates=[State.INIT],
          displayedName="Call ME", requiredAccessLevel=AccessLevel.EXPERT)
    def callme(self):
        pass

    @Slot(allowedStates=[State.ACTIVE],
          requiredAccessLevel=AccessLevel.OPERATOR)
    def yep(self):
        pass


def get_slot_proxy(additional=False, klass=SlottedDevice):
    schema = klass.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)

    device = DeviceProxy(device_id="dev", server_id="Test", binding=binding)
    slot_proxy = PropertyProxy(root_proxy=device, path="callme")
    if additional:
        return slot_proxy, PropertyProxy(root_proxy=device, path="yep")
    else:
        return slot_proxy


def test_command_controller_basics(gui_app):
    slot_proxy = get_slot_proxy()
    controller = DisplayCommand(proxy=slot_proxy)
    controller.create(None)

    assert controller.widget is not None
    assert isinstance(controller._button, QToolButton)
    assert controller._actions[0].action.text() == "Call ME"
    assert controller._actions[0].action.toolTip() == "dev.callme"

    controller.destroy()
    assert controller.widget is None


def test_command_controller_access_level():
    slot_proxy, add_slot_proxy = get_slot_proxy(additional=True,
                                                klass=AccessSlottedDevice)
    controller = DisplayCommand(proxy=slot_proxy)
    controller.create(None)
    assert controller.visualize_additional_property(add_slot_proxy)

    assert controller.widget is not None
    assert isinstance(controller._button, QToolButton)
    assert controller._actions[0].action.text() == "Call ME"
    assert controller._actions[0].action.toolTip() == "dev.callme"
    assert controller._actions[1].action.text() == "yep"
    assert controller._actions[1].action.toolTip() == "dev.yep"

    with access_level(AccessLevel.OBSERVER):
        controller.setEnabled(None)
        assert not controller._actions[0].action.isEnabled()
        assert not controller._actions[1].action.isEnabled()

    with access_level(AccessLevel.EXPERT):
        controller.setEnabled(None)
        assert controller._actions[0].action.isEnabled()
        # Wrong state, correct access level
        assert not controller._actions[1].action.isEnabled()
        # set new state
        set_proxy_value(slot_proxy, "state", State.ACTIVE.value)
        assert controller._actions[1].action.isEnabled()

    with access_level(AccessLevel.OPERATOR):
        controller.setEnabled(None)
        # Actice state
        assert not controller._actions[0].action.isEnabled()
        assert controller._actions[1].action.isEnabled()

        set_proxy_value(slot_proxy, "state", State.INIT.value)
        assert not controller._actions[0].action.isEnabled()
        assert not controller._actions[1].action.isEnabled()


def test_command_controller_trigger(gui_app, mocker):
    slot_proxy = get_slot_proxy()
    controller = DisplayCommand(proxy=slot_proxy)
    controller.create(None)
    network = mocker.Mock()
    with singletons(network=network):
        controller._actions[0].action.trigger()
        network.onExecute.assert_called_with("dev", "callme", False)


def test_command_controller_macro_trigger(gui_app, mocker):
    slot_proxy = get_slot_proxy()
    node = SystemTreeNode()
    node.attributes = {"type": "macro"}
    slot_proxy.root_proxy.topology_node = node
    controller = DisplayCommand(proxy=slot_proxy)
    controller.create(None)
    network = mocker.Mock()
    with singletons(network=network):
        controller._actions[0].action.trigger()
        network.onExecute.assert_called_with("dev", "callme", True)


def test_command_controller_additional_proxy(gui_app):
    slot_p1, slot_p2 = get_slot_proxy(additional=True)
    controller = DisplayCommand(proxy=slot_p1)
    controller.create(None)

    assert controller.visualize_additional_property(slot_p2)
    # no displayedName, should be path
    assert controller._actions[1].action.text() == "yep"
    assert len(controller._actions) == 2
    assert slot_p2 in controller.proxies
    assert slot_p2.key in controller.model.keys

    assert controller.remove_additional_property(slot_p2)
    assert len(controller._actions) == 1
    assert slot_p2 not in controller.proxies
    assert slot_p2.key not in controller.model.keys

    # Already removed
    assert not controller.remove_additional_property(slot_p2)


def test_command_controller_state_change(gui_app):
    schema = SlottedDevice.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)

    dev_proxy = DeviceProxy(device_id="dev", server_id="swerver",
                            binding=binding)
    slot_proxy = PropertyProxy(root_proxy=dev_proxy, path="yep")
    controller = DisplayCommand(proxy=slot_proxy)
    controller.create(None)

    # device state is INIT
    assert not controller._actions[0].action.isEnabled()

    set_proxy_value(slot_proxy, "state", State.ACTIVE.value)
    # now `yep` should be enabled
    assert controller._actions[0].action.isEnabled()


def test_command_controller_button_finalization(gui_app):
    slot1, slot2 = get_slot_proxy(additional=True, klass=EmptyDevice)

    slotSchema = SlottedDevice()

    controller = DisplayCommand(proxy=slot1)
    controller.create(None)
    controller.visualize_additional_property(slot2)

    assert controller._actions[0].action.text() == "NO TEXT"
    assert controller._actions[1].action.text() == "NO TEXT"

    # The device "comes online"
    schema = slotSchema.getClassSchema()
    build_binding(schema, existing=slot1.root_proxy.binding)

    assert controller._actions[0].action.text() == "Call ME"
    assert controller._actions[1].action.text() == "yep"

    # The device "has schema injection"
    slotSchema.callme.descriptor.displayedName = "Injected"
    schema = slotSchema.getClassSchema()
    build_binding(schema, existing=slot1.root_proxy.binding)
    assert controller._actions[0].action.text() == "Injected"
    assert controller._actions[1].action.text() == "yep"


def test_command_controller_confirmation_dialog(gui_app, mocker):
    slot_proxy = get_slot_proxy()
    controller = DisplayCommand(proxy=slot_proxy)
    controller.create(None)
    network = mocker.Mock()
    widget = controller.widget

    with singletons(network=network):
        controller.model.requires_confirmation = False
        button = controller._button
        click_button(controller._button)

        assert not controller.model.requires_confirmation
        assert network.onExecute.call_count == 1
        assert not controller.widget.font().bold()

    with singletons(network=network):
        controller._requires_confirmation_slot()
        assert controller.model.requires_confirmation
        assert "bold" in widget.styleSheet()
        if not IS_MAC_SYSTEM:
            sh = ("QToolButton{font-size: 10pt; font: bold;}"
                  "QToolButton:enabled{color: rgb(255, 145, 255);}")
            assert sh in widget.styleSheet()

        # We don"t want to execute
        QMessageBox.question = mocker.MagicMock(return_value=QMessageBox.No)
        click_button(button)
        assert network.onExecute.call_count == 1
        # We want to execute
        QMessageBox.question = mocker.MagicMock(return_value=QMessageBox.Yes)
        click_button(button)
        assert network.onExecute.call_count == 2

    controller._requires_confirmation_slot()
    if not IS_MAC_SYSTEM:
        style = "QToolButton { font-size: 10pt; font: normal }"
        assert style in widget.styleSheet()

    QMessageBox.question = mocker.MagicMock(return_value=QMessageBox.No)

    format_action = controller.widget.actions()[1]
    path = "karabogui.controllers.display.command.FormatLabelDialog"
    dialog = mocker.patch(path)
    assert controller.model.font_size == 10
    if not IS_MAC_SYSTEM:
        style = "QToolButton { font-size: 10pt; font: normal }"
        assert style in widget.styleSheet()
    dialog().font_size = 14
    dialog().font_weight = "bold"
    dialog().exec.return_value = QDialog.Accepted
    format_action.trigger()
    assert controller.model.font_size == 14
    if not IS_MAC_SYSTEM:
        style = "QToolButton { font-size: 14pt; font: bold }"
        assert style in widget.styleSheet()
