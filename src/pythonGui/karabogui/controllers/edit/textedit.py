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

from traits.api import Instance, Int

from karabo.common.scenemodel.api import LineEditModel
from karabogui.binding.api import (
    CharBinding, StringBinding, VectorCharBinding, get_editor_value)
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.util import SignalBlocker
from karabogui.widgets.hints import LineEdit

BINDING_TYPES = (CharBinding, StringBinding, VectorCharBinding)


@register_binding_controller(ui_name='Text Field', can_edit=True,
                             klassname='EditableLineEdit', priority=10,
                             binding_type=BINDING_TYPES)
class EditableLineEdit(BaseBindingController):
    model = Instance(LineEditModel, args=())
    # Internal details
    _last_cursor_pos = Int(0)

    def create_widget(self, parent):
        widget = LineEdit(parent)
        widget.textChanged.connect(self._on_text_changed)
        return widget

    def binding_update(self, proxy):
        if isinstance(proxy.binding, CharBinding):
            mask = 'X'
            with SignalBlocker(self.widget):
                self.widget.setInputMask(mask)

    def value_update(self, proxy):
        value = get_editor_value(proxy, '')
        if not isinstance(value, str):
            value = value.decode()

        with SignalBlocker(self.widget):
            self.widget.setText(value)
        self.widget.setCursorPosition(self._last_cursor_pos)

    def _on_text_changed(self, value):
        if self.proxy.binding is None:
            return
        self._last_cursor_pos = self.widget.cursorPosition()
        self.proxy.edit_value = value

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)
