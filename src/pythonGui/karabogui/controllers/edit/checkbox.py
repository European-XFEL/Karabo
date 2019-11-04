#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QCheckBox
from traits.api import Instance, Undefined

from karabo.common.scenemodel.api import CheckBoxModel
from karabogui import globals as krb_globals
from karabogui.binding.api import BoolBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import SignalBlocker


@register_binding_controller(ui_name='Toggle Field', can_edit=True,
                             klassname='EditableCheckBox',
                             binding_type=BoolBinding, priority=10)
class EditableCheckBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(CheckBoxModel, args=())

    def create_widget(self, parent):
        widget = QCheckBox(parent)
        widget.setFocusPolicy(Qt.StrongFocus)
        widget.stateChanged.connect(self._on_user_edit)
        return widget

    def value_update(self, proxy):
        checkState = Qt.Checked if get_editor_value(proxy) else Qt.Unchecked
        with SignalBlocker(self.widget):
            self.widget.setCheckState(checkState)

    def state_update(self, proxy):
        root_proxy = proxy.root_proxy
        value = root_proxy.state_binding.value
        if value is Undefined or not value:
            return

        binding = proxy.binding
        is_allowed = binding.is_allowed(value)
        is_accessible = (krb_globals.GLOBAL_ACCESS_LEVEL >=
                         binding.required_access_level)

        self.widget.setEnabled(is_allowed and is_accessible)

    # @pyqtSlot(int)
    def _on_user_edit(self, state):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = (state == Qt.Checked)
