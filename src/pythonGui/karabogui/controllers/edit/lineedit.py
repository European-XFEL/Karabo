#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QLineEdit
from traits.api import Instance, Int

from karabo.common.scenemodel.api import LineEditModel
from karabogui.binding.api import CharBinding, StringBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.util import SignalBlocker


@register_binding_controller(ui_name='Text Field', can_edit=True,
                             klassname='EditableLineEdit', priority=10,
                             binding_type=(CharBinding, StringBinding))
class EditableLineEdit(BaseBindingController):
    # The scene model class used by this controlelr
    model = Instance(LineEditModel)
    # Internal details
    _last_cursor_pos = Int(0)

    def create_widget(self, parent):
        widget = QLineEdit(parent)
        widget.textChanged.connect(self._on_text_changed)
        return widget

    def value_update(self, proxy):
        value = proxy.value
        if not isinstance(value, str):
            value = value.decode()

        with SignalBlocker(self.widget):
            self.widget.setText(value)
        self.widget.setCursorPosition(self._last_cursor_pos)

    @pyqtSlot(str)
    def _on_text_changed(self, value):
        self._last_cursor_pos = self.widget.cursorPosition()
        if self.proxy.binding is not None:
            self.proxy.value = value
