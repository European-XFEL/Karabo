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
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.native import Configurable, VectorString
from karabogui.binding.api import apply_default_configuration
from karabogui.const import IS_MAC_SYSTEM
from karabogui.dialogs.listedit import ListEditDialog
from karabogui.testing import get_class_property_proxy

from ..strlist import EditableListElement


class Object(Configurable):
    prop = VectorString(defaultValue=["hi there"])


class ListEditMock(ListEditDialog):
    @property
    def values(self):
        return ["foo", "bar"]

    def exec(self):
        return QDialog.Accepted


def test_strlist_basics(gui_app, mocker):
    # setup
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    controller = EditableListElement(proxy=proxy)
    controller.create(None)
    apply_default_configuration(proxy.root_proxy.binding)\

    # focus policy
    if IS_MAC_SYSTEM:
        assert controller.widget.focusPolicy() == Qt.TabFocus
    else:
        assert controller.widget.focusPolicy() == Qt.StrongFocus

    # edit dialog
    target = "karabogui.controllers.edit.strlist.ListEditDialog"
    mocker.patch(target, new=ListEditMock)
    controller._on_edit_clicked()
    assert proxy.edit_value == ["foo", "bar"]

    # teardown
    controller.destroy()
    assert controller.widget is None
