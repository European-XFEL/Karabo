#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog, QPushButton
from traits.api import Instance

from karabo.common.scenemodel.api import EditableListElementModel
from karabogui.binding.api import VectorStringBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.listedit import ListEdit
from karabogui.controllers.registry import register_binding_controller


@register_binding_controller(ui_name='List Element Field', can_edit=True,
                             klassname='EditableListElement',
                             binding_type=VectorStringBinding)
class EditableListElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(EditableListElementModel)

    def create_widget(self, parent):
        widget = QPushButton('Edit list', parent)
        widget.setStyleSheet('QPushButton { text-align: left; }')
        widget.clicked.connect(self._on_edit_clicked)
        return widget

    @pyqtSlot()
    def _on_edit_clicked(self):
        if self.proxy.binding is None:
            return

        list_edit = ListEdit(self.proxy, True)
        list_edit.set_texts('Add', '&Name', 'Edit')
        if list_edit.exec_() == QDialog.Accepted:
            self.proxy.edit_value = list_edit.values
