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
from qtpy.QtWidgets import QDialog, QToolButton

from karabo.common.scenemodel.api import (
    EditableListModel, EditableRegexListModel)
from karabo.native import (
    Configurable, VectorInt8, VectorInt32, VectorRegexString)
from karabogui.binding.api import apply_default_configuration, get_min_max_size
from karabogui.dialogs.listedit import ListEditDialog
from karabogui.testing import (
    click_button, get_class_property_proxy, set_proxy_value)

from ..list import EditableList, EditableRegexList


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1])


class SizeObject(Configurable):
    prop = VectorInt32(defaultValue=[1], minSize=1, maxSize=3)


class ShortObject(Configurable):
    prop = VectorInt8(defaultValue=[1])


class RegexObject(Configurable):
    prop = VectorRegexString(
        defaultValue=["remote:output"],
        regex=r"^[A-Za-z0-9_-]{1,60}(:)[A-Za-z0-9_-]{1,60}$")


class ListEditMock(ListEditDialog):
    @property
    def values(self):
        return ["-1", "42", "-1"]

    def exec(self):
        return QDialog.Accepted


@pytest.fixture
def list_setup(gui_app):
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = EditableList(proxy=proxy, model=EditableListModel())
    controller.create(None)
    apply_default_configuration(proxy.root_proxy.binding)
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_list_focus_policy(list_setup):
    controller, _ = list_setup
    assert controller._internal_widget.focusPolicy() == Qt.StrongFocus


def test_list_set_value(list_setup):
    controller, proxy = list_setup
    controller.last_cursor_position = 0
    set_proxy_value(proxy, "prop", [0, 2])
    assert controller._internal_widget.text() == "0,2"
    assert controller._internal_widget.cursorPosition() == 0


def test_list_edit_value(list_setup):
    controller, proxy = list_setup
    controller.set_read_only(False)
    controller._internal_widget.setText("3,4")
    assert all(proxy.edit_value == [3, 4])


def test_list_edit_empty_value(list_setup):
    controller, proxy = list_setup
    controller._internal_widget.setText("NOT IMPORTANT")
    controller.set_read_only(False)
    controller._internal_widget.setText("")
    value = proxy.edit_value
    assert len(value) == 0 and all(value == [])


def test_list_edit_dialog(list_setup, mocker):
    controller, proxy = list_setup
    target = "karabogui.controllers.edit.list.ListEditDialog"
    mocker.patch(target, new=ListEditMock)
    controller.set_read_only(False)
    controller._on_edit_clicked()
    assert all(proxy.edit_value == [-1, 42, -1])


@pytest.fixture
def list_size_setup(gui_app):
    size_proxy = get_class_property_proxy(SizeObject.getClassSchema(), "prop")
    size_controller = EditableList(proxy=size_proxy,
                                   model=EditableListModel())
    size_controller.create(None)
    apply_default_configuration(size_proxy.root_proxy.binding)
    yield size_controller, size_proxy
    # teardown
    size_controller.destroy()
    assert size_controller.widget is None


def test_size_list(list_size_setup):
    size_controller, size_proxy = list_size_setup
    assert get_min_max_size(size_proxy.binding) == (1, 3)
    size_controller.set_read_only(False)
    size_controller._internal_widget.setText("3,4")
    assert all(size_proxy.edit_value == [3, 4])
    size_controller._internal_widget.setText("3,4,5,6")
    assert size_proxy.edit_value is None
    size_controller._internal_widget.setText("")
    assert size_proxy.edit_value is None


def test_size_list_trailing_zeros(list_size_setup):
    size_controller, size_proxy = list_size_setup
    assert get_min_max_size(size_proxy.binding) == (1, 3)
    size_controller.set_read_only(False)
    size_controller._internal_widget.setText("03,4")
    assert size_proxy.edit_value is None
    size_controller._internal_widget.setText("3,4")
    assert all(size_proxy.edit_value == [3, 4])
    size_controller._internal_widget.setText("000000,4")
    assert all(size_proxy.edit_value == [0, 4])


def test_list_shorts(gui_app):
    # setup
    short_proxy = get_class_property_proxy(ShortObject.getClassSchema(),
                                           "prop")
    short_controller = EditableList(proxy=short_proxy,
                                    model=EditableListModel())
    short_controller.create(None)
    apply_default_configuration(short_proxy.root_proxy.binding)
    # test body
    short_controller.set_read_only(False)
    short_controller._internal_widget.setText("03,4")
    assert short_proxy.edit_value is None
    short_controller._internal_widget.setText("3,4")
    assert all(short_proxy.edit_value == [3, 4])
    # out of range
    short_controller._internal_widget.setText("10300")
    assert short_proxy.edit_value is None
    # teardown
    short_controller.destroy()
    assert short_controller.widget is None


@pytest.fixture
def editable_regex_list_setup(gui_app):
    proxy = get_class_property_proxy(RegexObject.getClassSchema(), "prop")
    controller = EditableRegexList(proxy=proxy, model=EditableRegexListModel())
    controller.create(None)
    controller.binding_update(proxy)
    apply_default_configuration(proxy.root_proxy.binding)
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_editable_regex_list_binding_update_validator(
        editable_regex_list_setup):
    controller, _ = editable_regex_list_setup
    assert controller._internal_widget.validator() is not None


def test_editable_regex_list_set_value(editable_regex_list_setup):
    controller, proxy = editable_regex_list_setup
    controller.last_cursor_position = 0
    set_proxy_value(proxy, "prop", ["remote:outputDaq"])
    assert controller._internal_widget.text() == "remote:outputDaq"
    assert controller._internal_widget.cursorPosition() == 0


def test_editable_regex_list_edit_value(editable_regex_list_setup):
    controller, proxy = editable_regex_list_setup
    controller.set_read_only(False)
    controller._internal_widget.setText("remote:outputDaq,remote:output")
    assert proxy.edit_value == ["remote:outputDaq", "remote:output"]


def test_editable_regex_list_edit_empty_value(editable_regex_list_setup):
    controller, proxy = editable_regex_list_setup
    controller.set_read_only(False)
    controller._internal_widget.setText("remote:output")
    assert proxy.edit_value == ["remote:output"]
    controller._internal_widget.setText("")
    value = proxy.edit_value
    assert value == []


def test_editable_list_dialog_value(editable_regex_list_setup, mocker):
    controller, proxy = editable_regex_list_setup
    controller.last_cursor_position = 0
    set_proxy_value(proxy, "prop", ["1", "2", "3"])
    button = controller.widget.findChild(QToolButton)
    path = "karabogui.controllers.edit.list.ListEditDialog"
    dialog = mocker.patch(path)
    dialog().exec.return_value = QDialog.Accepted
    dialog().values = ["1", "2", "3", "4"]
    click_button(button)
    assert controller._internal_widget.text() == "1,2,3,4"
    assert controller._internal_widget.cursorPosition() == 0
