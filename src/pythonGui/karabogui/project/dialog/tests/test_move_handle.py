# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from qtpy.QtCore import Qt

from karabo.common.project.api import MacroModel
from karabogui.project.dialog.move_handle import MoveHandleDialog
from karabogui.testing import click_button


def test_move_handle_dialog(gui_app):
    models = [MacroModel(simple_name="Macro1"),
              MacroModel(simple_name="Macro2")]

    dialog = MoveHandleDialog(models)
    assert len(dialog.items) == 2

    item = dialog.widget.item(0)
    assert item.data(Qt.UserRole).simple_name == "Macro1"

    # move item back and forth
    dialog.widget.setCurrentRow(1)
    click_button(dialog.move_up_button)
    item = dialog.widget.item(0)
    assert item.data(Qt.UserRole).simple_name == "Macro2"
    click_button(dialog.move_down_button)
    item = dialog.widget.item(0)
    assert item.data(Qt.UserRole).simple_name == "Macro1"
    dialog.done(1)
    dialog.done(0)
    dialog.close()
    assert not dialog.isModal()
