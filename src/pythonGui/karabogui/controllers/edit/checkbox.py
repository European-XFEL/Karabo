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
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QCheckBox
from traits.api import Instance

from karabo.common.scenemodel.api import CheckBoxModel
from karabogui.binding.api import BoolBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
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
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)

    def _on_user_edit(self, state):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = (state == Qt.Checked)
