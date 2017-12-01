#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QCheckBox
from traits.api import Instance

from karabo.common.scenemodel.api import CheckBoxModel
from karabogui.binding.api import BoolBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller


@register_binding_controller(ui_name='Toggle Field', can_edit=True,
                             klassname='EditableCheckBox',
                             binding_type=BoolBinding, priority=10)
class EditableCheckBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(CheckBoxModel)

    def create_widget(self, parent):
        widget = QCheckBox(parent)
        widget.setFocusPolicy(Qt.StrongFocus)
        widget.stateChanged.connect(self._on_user_edit)
        return widget

    def value_update(self, proxy):
        checkState = Qt.Checked if proxy.value else Qt.Unchecked
        self.widget.setCheckState(checkState)

    @pyqtSlot(int)
    def _on_user_edit(self, state):
        self.proxy.value = (state == Qt.Checked)
