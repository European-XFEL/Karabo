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
from qtpy.QtWidgets import QComboBox
from traits.api import Instance

from karabo.common.scenemodel.api import EditableComboBoxModel
from karabogui.binding.api import BaseBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.util import MouseWheelEventBlocker, SignalBlocker


def _is_compatible(binding):
    return len(binding.options) > 0


@register_binding_controller(ui_name="Selection Field", can_edit=True,
                             klassname="EditableComboBox",
                             binding_type=BaseBinding,
                             is_compatible=_is_compatible, priority=20)
class EditableComboBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(EditableComboBoxModel, args=())
    # Internal traits
    _filter = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)

        self._filter = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._filter)
        widget.currentIndexChanged[int].connect(self._on_user_edit)
        widget.setFocusPolicy(Qt.StrongFocus)
        return widget

    def binding_update(self, proxy):
        with SignalBlocker(self.widget):
            self.widget.clear()
            self.widget.addItems([str(o) for o in proxy.binding.options])
            self.widget.setCurrentIndex(-1)

    def value_update(self, proxy):
        value = get_editor_value(proxy)
        if value is None:
            return
        options = proxy.binding.options
        try:
            index = next(i for i, v in enumerate(options)
                         if v == value)
        except StopIteration:
            index = -1
        with SignalBlocker(self.widget):
            self.widget.setCurrentIndex(index)

    def _on_user_edit(self, index):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = self.proxy.binding.options[index]

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)
