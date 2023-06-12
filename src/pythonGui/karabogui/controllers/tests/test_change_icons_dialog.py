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
from karabo.common.api import KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_INC
from karabogui import messagebox
from karabogui.binding.api import FloatBinding, StringBinding, Uint64Binding
from karabogui.controllers import icons_dialogs
from karabogui.testing import click_button


def test_digit_dialog_threshold(gui_app):
    MAX_UINT64 = 0xFFFFFFFFFFFFFFFF

    # 1. Floats
    binding = FloatBinding(attributes={
        KARABO_SCHEMA_MIN_INC: 3.0,
        KARABO_SCHEMA_MAX_INC: MAX_UINT64})
    dialog = icons_dialogs.DigitDialog([], binding)
    assert dialog.value.validator().minInc == 3.0
    assert dialog.value.validator().maxInc == MAX_UINT64

    # 2. Integers
    binding = Uint64Binding(attributes={
        KARABO_SCHEMA_MIN_INC: 2,
        KARABO_SCHEMA_MAX_INC: MAX_UINT64}
    )
    dialog = icons_dialogs.DigitDialog([], binding)
    assert dialog.value.validator().minInc == 2
    # Integers have a 32 bit limitation
    assert dialog.value.validator().maxInc == MAX_UINT64


def test_digit_dialog_order_of_items(gui_app, mocker):
    """Test the order of items added.They should always be sorted."""
    dialog = icons_dialogs.DigitDialog([], FloatBinding())
    dialog.lessEqual.setChecked(True)

    for val in (10.0, 0.0, 20.0):
        dialog.value.setText(str(val))
        click_button(dialog.addValue)
    valueList = dialog.valueList
    items = [valueList.item(i).text() for i in range(valueList.count())]
    assert items == ["<= 0.0", "<= 10.0", "<= 20.0"]

    # Add existing value with different operator - "<"
    dialog.less.setChecked(True)
    dialog.value.setText("10.0")
    click_button(dialog.addValue)
    items = [valueList.item(i).text() for i in range(valueList.count())]
    assert items == ["<= 0.0", "< 10.0", "<= 10.0", "<= 20.0"]

    # Add existing value with same operator.
    mock_error = mocker.patch.object(messagebox, "show_error")
    click_button(dialog.addValue)
    message = "Cannot add new condition; it already exists."
    mock_error.assert_called_once_with(message, parent=dialog)


def test_text_dialog_options(gui_app):
    """String property with options"""
    binding = StringBinding(
        attributes={"options": ["one", "two", "three"]})
    dialog = icons_dialogs.TextDialog(items=[], binding=binding)

    assert dialog.stack.currentWidget() == dialog.textOptionsPage
    value_list = dialog.valueList
    # The binding options are showed in a combobox and not added to the
    # list widget.
    assert value_list.count() == 0
    assert len(dialog.itemsComboBox) == 3
    for i in range(3):
        assert dialog.itemsComboBox.itemText(i) == binding.options[i]

    # The items in the controller's model are added to the list widget.
    item1 = icons_dialogs.IconItem(value="one")
    item2 = icons_dialogs.IconItem(value="two")

    dialog = icons_dialogs.TextDialog(items=[item1, item2],
                                      binding=binding)
    value_list = dialog.valueList
    assert value_list.count() == 2


def test_text_dialog_no_options(gui_app):
    """String property without options"""
    binding = StringBinding()
    dialog = icons_dialogs.TextDialog(items=[], binding=binding)
    assert dialog.stack.currentWidget() == dialog.textPage

    value_list = dialog.valueList
    items = [value_list.item(i).text() for i in range(value_list.count())]
    assert items == []
