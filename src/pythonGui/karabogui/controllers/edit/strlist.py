#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
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
#############################################################################
from qtpy.QtWidgets import QDialog, QPushButton
from traits.api import Instance

from karabo.common.scenemodel.api import EditableListElementModel
from karabogui.binding.api import VectorStringBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.dialogs.listedit import ListEditDialog


@register_binding_controller(ui_name="List Element Field", can_edit=True,
                             klassname="EditableListElement",
                             binding_type=VectorStringBinding)
class EditableListElement(BaseBindingController):
    model = Instance(EditableListElementModel, args=())

    def create_widget(self, parent):
        widget = QPushButton("Edit list", parent)
        widget.setStyleSheet("QPushButton { text-align: center; }")
        widget.clicked.connect(self._on_edit_clicked)
        return widget

    def _on_edit_clicked(self):
        binding = self.proxy.binding
        if binding is None:
            return

        list_edit = ListEditDialog(binding, duplicates_ok=True,
                                   parent=self.widget)
        list_edit.set_list(get_editor_value(self.proxy, []))
        if list_edit.exec() == QDialog.Accepted:
            self.proxy.edit_value = list_edit.values

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)
