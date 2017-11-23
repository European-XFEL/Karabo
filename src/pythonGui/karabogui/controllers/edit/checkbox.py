#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QCheckBox
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import CheckBoxModel
from karabogui.binding.api import (
    BaseBindingController, BoolBinding, register_binding_controller
)


# XXX: priority = 10
@register_binding_controller(ui_name='Toggle Field', binding_type=BoolBinding)
class EditableCheckBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(CheckBoxModel)

    def create_widget(self, parent):
        widget = QCheckBox(parent)
        widget.setFocusPolicy(Qt.StrongFocus)
        widget.stateChanged.connect(self._on_user_edit)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        checkState = Qt.Checked if value else Qt.Unchecked
        self.widget.setCheckState(checkState)

    @pyqtSlot(int)
    def _on_user_edit(self, state):
        self.proxy.value = (state == Qt.Checked)
