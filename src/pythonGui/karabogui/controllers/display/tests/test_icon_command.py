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
from qtpy.QtWidgets import QToolButton

from karabo.common.states import State
from karabo.native import AccessLevel, Configurable, Slot, String
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, apply_default_configuration, build_binding)
from karabogui.testing import set_proxy_value, singletons


class EmptyDevice(Configurable):
    """What a device looks like to a scene when it's offline"""


class SlottedDevice(Configurable):
    state = String(defaultValue=State.INIT)

    @Slot(allowedStates=[State.INIT],
          displayedName="Call ME",
          requiredAccessLevel=AccessLevel.OBSERVER)
    def callme(self):
        pass

    @Slot(allowedStates=[State.ACTIVE],
          requiredAccessLevel=AccessLevel.OBSERVER)
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


@pytest.fixture
def icon_command_setup(gui_app):
    # import required at this point
    from ..icon_command import DisplayIconCommand

    # setup
    slot_proxy = get_slot_proxy()
    controller = DisplayIconCommand(proxy=slot_proxy)
    controller.create(None)
    yield controller
    # teardown
    controller.destroy()
    assert controller.widget is None


@pytest.fixture
def icon_command_dialog_setup(gui_app):
    from ..icon_command import IconSelectionDialog
    dialog = IconSelectionDialog()
    yield dialog
    dialog.destroy()


def test_icon_command_basics(icon_command_setup):
    """Test the basics of the display icon command"""
    controller = icon_command_setup
    assert controller.widget is not None
    assert isinstance(controller._button, QToolButton)


def test_icon_command_trigger(icon_command_setup, mocker):
    controller = icon_command_setup
    network = mocker.Mock()
    with singletons(network=network):
        controller._button.clicked.emit(True)
        network.onExecute.assert_called_with("dev", "callme", False)


def test_icon_command_state_change(gui_app):
    from ..icon_command import DisplayIconCommand
    schema = SlottedDevice.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)

    dev_proxy = DeviceProxy(device_id="dev", server_id="swerver",
                            binding=binding)
    slot_proxy = PropertyProxy(root_proxy=dev_proxy, path="yep")
    controller = DisplayIconCommand(proxy=slot_proxy)
    controller.create(None)

    # device state is INIT
    assert not controller._button.isEnabled()

    set_proxy_value(slot_proxy, "state", State.ACTIVE.value)
    # now `yep` should be enabled
    assert controller._button.isEnabled()

    controller.setEnabled(False)
    assert not controller._button.isEnabled()

    controller.setEnabled(True)
    assert controller._button.isEnabled()


def test_icon_command_icon_change(icon_command_setup, mocker):
    from ..icon_command import BUTTON_ICONS
    controller = icon_command_setup

    actions = controller.widget.actions()
    assert len(actions) == 1

    exec_path = ("karabogui.controllers.display.icon_command."
                 "IconSelectionDialog.exec")
    new_icon = "Left"

    def exec_dialog(self):
        self.icon = BUTTON_ICONS[new_icon]
        self.name = new_icon

    mocker.patch(exec_path, exec_dialog)
    actions[0].trigger()
    assert controller._icon == BUTTON_ICONS[new_icon]
    assert controller.model.icon_name == new_icon


def test_dialog_init(icon_command_dialog_setup):
    from ..icon_command import BUTTON_ICONS, NO_SELECTION

    dialog = icon_command_dialog_setup

    assert dialog.icon == NO_SELECTION
    assert dialog.name == ""

    model = dialog.list_view.model()
    for row in range(model.rowCount()):
        index = model.index(row, 0)
        name = index.data(Qt.DisplayRole)

        assert name in BUTTON_ICONS


def test_dialog_valid_double_click(icon_command_dialog_setup, mocker):
    from ..icon_command import BUTTON_ICONS
    new_icon = "Left"

    def mocked_returns(role):
        if role == Qt.DisplayRole:
            return new_icon
        if role == Qt.UserRole + 1:
            return BUTTON_ICONS[new_icon]

    dialog = icon_command_dialog_setup

    mocked_index = mocker.Mock()
    mocked_index.data.side_effect = mocked_returns
    dialog.handleDoubleClick(mocked_index)

    assert dialog.icon == BUTTON_ICONS[new_icon]
    assert dialog.name == new_icon


def test_dialog_invalid_double_click(icon_command_dialog_setup, mocker):
    from ..icon_command import NO_SELECTION

    def mocked_returns(role):
        if role == Qt.DisplayRole:
            return None
        if role == Qt.UserRole + 1:
            return None

    dialog = icon_command_dialog_setup

    mocked_index = mocker.Mock()
    mocked_index.data.side_effect = mocked_returns
    dialog.handleDoubleClick(mocked_index)

    assert dialog.icon == NO_SELECTION
    assert dialog.name == ""
