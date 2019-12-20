#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtWidgets import QDialog, QPushButton
from traits.api import Instance

from karabo.common.scenemodel.api import EditableListElementModel
from karabogui.binding.api import VectorStringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.dialogs.listedit import ListEditDialog


@register_binding_controller(ui_name='List Element Field', can_edit=True,
                             klassname='EditableListElement',
                             binding_type=VectorStringBinding)
class EditableListElement(BaseBindingController):
    model = Instance(EditableListElementModel, args=())

    def create_widget(self, parent):
        widget = QPushButton('Edit list', parent)
        widget.setStyleSheet('QPushButton { text-align: center; }')
        widget.clicked.connect(self._on_edit_clicked)
        return widget

    def _on_edit_clicked(self):
        if self.proxy.binding is None:
            return

        list_edit = ListEditDialog(self.proxy, duplicates_ok=True,
                                   parent=self.widget)
        list_edit.set_texts('Add', '&Name', 'Edit')
        if list_edit.exec_() == QDialog.Accepted:
            self.proxy.edit_value = list_edit.values
