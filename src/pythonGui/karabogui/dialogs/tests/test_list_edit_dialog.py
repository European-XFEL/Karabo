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

from karabogui.binding.api import (
    VectorBoolBinding, VectorDoubleBinding, VectorInt64Binding,
    VectorStringBinding)
from karabogui.controllers.table.api import string2list
from karabogui.dialogs.api import ListEditDialog


@pytest.fixture
def dialogs(gui_app):
    string_binding = VectorStringBinding()
    string_dialog = ListEditDialog(binding=string_binding)
    string_dialog.set_list(["1", "2"])

    bool_binding = VectorBoolBinding()
    bool_dialog = ListEditDialog(binding=bool_binding)
    bool_dialog.set_list([True, False, True])

    int_binding = VectorInt64Binding()
    int_dialog = ListEditDialog(binding=int_binding)
    int_dialog.set_list([1, 2, 3])

    double_binding = VectorDoubleBinding()
    double_dialog = ListEditDialog(binding=double_binding)
    double_dialog.set_list([1.0, 2.0, 3.0])

    return string_dialog, bool_dialog, int_dialog, double_dialog


def test_allowed_choices(dialogs):

    string_dialog, bool_dialog, int_dialog, double_dialog = dialogs

    assert string_dialog._allowed_choices == {}
    assert int_dialog._allowed_choices == {}
    assert bool_dialog._allowed_choices == {"False": False, "True": True}
    assert double_dialog._allowed_choices == {}

    for dialog in dialogs:
        assert dialog.windowTitle() == "Edit list"


def test_set_list(dialogs):
    string_dialog, bool_dialog, int_dialog, double_dialog = dialogs

    assert string_dialog.list_widget.count() == 2
    string_dialog.set_list(["1", "2", "3"])
    assert string_dialog.list_widget.count() == 3
    assert string_dialog.values == ["1", "2", "3"]

    assert bool_dialog.list_widget.count() == 3
    assert bool_dialog.values == [True, False, True]
    bool_dialog.set_list([])
    assert bool_dialog.list_widget.count() == 0
    assert bool_dialog.values == []

    assert int_dialog.list_widget.count() == 3
    int_dialog.set_list([1, 2, 3, 4, 5])
    assert int_dialog.list_widget.count() == 5
    assert int_dialog.values == [1, 2, 3, 4, 5]

    assert double_dialog.list_widget.count() == 3
    double_dialog.set_list([1.1, 2.2, 3.3, 4.4, 5.5])
    assert double_dialog.list_widget.count() == 5
    assert double_dialog.values == [1.1, 2.2, 3.3, 4.4, 5.5]


def test_on_add_slot(dialogs, mocker):
    """Test the add slot of the list edit dialog"""
    string_dialog, bool_dialog, int_dialog, double_dialog = dialogs

    mock_input_dialog = mocker.patch("karabogui.dialogs.listedit.QInputDialog")

    mock_input_dialog.getText.return_value = "new", True
    string_dialog._add_clicked()
    assert string_dialog.list_widget.count() == 3
    assert string_dialog.values == ["1", "2", "new"]

    mock_input_dialog.getItem.return_value = "False", True
    bool_dialog._add_clicked()
    assert bool_dialog.list_widget.count() == 4
    assert bool_dialog.values == [True, False, True, False]

    mock_input_dialog.getInt.return_value = 55, True
    int_dialog._add_clicked()
    assert int_dialog.list_widget.count() == 4
    assert int_dialog.values == [1, 2, 3, 55]

    mock_input_dialog.getDouble.return_value = 42.2, True
    double_dialog._add_clicked()
    assert double_dialog.list_widget.count() == 4
    assert double_dialog.values == [1.0, 2.0, 3.0, 42.2]


def test_on_remove_slot(dialogs):
    """Test the remove slot of the list edit dialog"""
    for dialog in dialogs:
        expected_values = dialog.values[1:]
        dialog.list_widget.setCurrentRow(0)
        dialog._remove_clicked()
        assert dialog.list_widget.count() == len(expected_values)
        assert dialog.values == expected_values


def test_move_up_slot(dialogs):
    """Move up button on second element should move it up by one row"""
    for dialog in dialogs:
        values = dialog.values
        values[1], values[0] = values[0], values[1]
        dialog.list_widget.setCurrentRow(1)
        dialog._move_up_clicked()
        assert values == dialog.values


def test_move_down_slot(dialogs):
    """Move down button on first element should move it down by one row"""
    for dialog in dialogs:
        values = dialog.values
        values[0], values[1] = values[1], values[0]
        dialog.list_widget.setCurrentRow(0)
        dialog._move_down_clicked()
        assert values == dialog.values


def test_edit_slot(dialogs, mocker):
    string_dialog, bool_dialog, int_dialog, double_dialog = dialogs

    mock_input_dialog = mocker.patch("karabogui.dialogs.listedit.QInputDialog")

    string_dialog.list_widget.setCurrentRow(0)
    mock_input_dialog.getText.return_value = "new", True
    string_dialog._edit_clicked()
    assert string_dialog.values == ["new", "2"]

    bool_dialog.list_widget.setCurrentRow(0)
    # Item dialog with keys of choices, we return first choice
    mock_input_dialog.getItem.return_value = "False", True
    bool_dialog._edit_clicked()
    assert bool_dialog.values == [False, False, True]

    int_dialog.list_widget.setCurrentRow(0)
    mock_input_dialog.getInt.return_value = 55, True
    int_dialog._edit_clicked()
    assert int_dialog.values == [55, 2, 3]

    double_dialog.list_widget.setCurrentRow(0)
    mock_input_dialog.getDouble.return_value = 42.2, True
    double_dialog._edit_clicked()
    assert double_dialog.values == [42.2, 2.0, 3.0]


def test_binding_validation(dialogs):
    for dialog in dialogs:
        binding = dialog.binding
        values = string2list(dialog.string_values)
        assert binding.validate_trait("value", values) is not None, \
            f"Validation of {values} for {binding} failed"
