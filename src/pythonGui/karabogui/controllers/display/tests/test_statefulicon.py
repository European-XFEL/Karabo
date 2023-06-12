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
from qtpy.QtWidgets import QDialog, QWidget

from karabo.common.api import State
from karabo.common.scenemodel.api import StatefulIconWidgetModel
from karabo.native import Configurable, String
from karabogui.indicators import STATE_COLORS
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..statefulicon import ICONS, StatefulIconWidget

ICON_NAME = "icon_bdump"


class MockSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class MockDialog(QDialog):
    singleton = None

    def exec(self):
        # Yeah... We need to break into the widget's code
        MockDialog.singleton = self


class Object(Configurable):
    state = String(displayType="State")


@pytest.fixture
def statefulicon_setup(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "state")
    yield proxy


def test_statefulicon_basics(statefulicon_setup):
    proxy = statefulicon_setup
    model = StatefulIconWidgetModel(icon_name=ICON_NAME)
    controller = StatefulIconWidget(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None

    controller.destroy()
    assert controller.widget is None


def test_statefulicon_set_value(statefulicon_setup, mocker):
    target = "karabogui.controllers.display.statefulicon.QSvgWidget"
    mocker.patch(target, new=MockSvgWidget)
    proxy = statefulicon_setup
    model = StatefulIconWidgetModel(icon_name=ICON_NAME)
    controller = StatefulIconWidget(proxy=proxy, model=model)
    controller.create(None)

    states = ("CHANGING", "ACTIVE", "PASSIVE", "DISABLED", "STATIC",
              "RUNNING", "NORMAL", "ERROR", "INIT", "UNKNOWN")

    for state in states:
        set_proxy_value(proxy, "state", state)
        color = STATE_COLORS[getattr(State, state)]
        svg = controller._icon.with_color(color)
        assert (controller.widget.loaded_data ==
                bytearray(svg, encoding="UTF-8"))


def test_statefulicon_pick_icon(statefulicon_setup, mocker):
    target = "karabogui.controllers.display.statefulicon.QDialog"
    mocker.patch(target, new=MockDialog)
    proxy = statefulicon_setup
    model = StatefulIconWidgetModel()
    controller = StatefulIconWidget(proxy=proxy, model=model)
    controller.create(None)

    # XXX: Really dig around inside the brains of the dialog opened
    # by controller._show_icon_picker.
    assert MockDialog.singleton is not None
    iconlist = MockDialog.singleton.children()[0]  # QListView
    item_model = iconlist.model()  # QStandardItemModel
    item_index = item_model.index(5, 0)  # QModelIndex
    # Mimic the user double-clicking
    iconlist.doubleClicked.emit(item_index)

    # Test that picking an icon actually set it to the scene model
    icon = item_index.data(Qt.UserRole + 1)  # QIcon
    assert controller._icon is icon
    assert model.icon_name == icon.name


def test_statefulicon_pick_icon_bailout(statefulicon_setup, mocker):
    """Test the bailout of an icon dialog"""
    target = "karabogui.controllers.display.statefulicon.QDialog"
    mocker.patch(target, new=MockDialog)
    proxy = statefulicon_setup
    model = StatefulIconWidgetModel()
    controller = StatefulIconWidget(proxy=proxy, model=model)
    assert controller._icon is None
    controller.create(None)
    assert MockDialog.singleton is not None
    no_selection = ICONS["icon_default"]
    assert controller._icon == no_selection
    assert model.icon_name == no_selection.name


def test_statefulicon_pick_not_supported_icon(statefulicon_setup, mocker):
    """Test that we maintain a not supported icon"""
    target = ("karabogui.controllers.display.statefulicon."
              "StatefulIconWidget._show_icon_picker")
    picker = mocker.patch(target)
    proxy = statefulicon_setup
    model = StatefulIconWidgetModel(icon_name="NoStateFullIcon")
    controller = StatefulIconWidget(proxy=proxy, model=model)
    controller.create(None)
    # dialog was not launched since we have an icon name!
    picker.assert_not_called()
    # The icon name is not found, the widget shows default icon
    assert controller._icon == ICONS["icon_default"]
    # However, model is not changed and keep icon name
    assert model.icon_name == "NoStateFullIcon"
