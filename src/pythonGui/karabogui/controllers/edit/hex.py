#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from numpy import log2
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QLineEdit
from traits.api import Instance

from karabo.common.scenemodel.api import HexadecimalModel
from karabogui.binding.api import IntBinding, get_editor_value, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    add_unit_label, BaseBindingController, is_proxy_allowed,
    register_binding_controller)
from karabogui.util import SignalBlocker


@register_binding_controller(ui_name='Hexadecimal', can_edit=True,
                             klassname='Hexadecimal', binding_type=IntBinding)
class Hexadecimal(BaseBindingController):
    model = Instance(HexadecimalModel, args=())

    _internal_widget = Instance(QLineEdit)

    def create_widget(self, parent):
        self._internal_widget = QLineEdit(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.setFocusPolicy(Qt.StrongFocus)
        self._internal_widget.textChanged.connect(self._on_user_edit)

        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)
        widget.setFocusProxy(self._internal_widget)

        return widget

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        num_bits = log2(max(abs(high), abs(low)))
        mask = 'h' * int(num_bits // 4 + 1)
        if low < 0:
            mask = "#" + mask
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setInputMask(mask)

    def value_update(self, proxy):
        self.widget.update_label(proxy)
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self._internal_widget):
                self._internal_widget.setText("{:x}".format(value))

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self._internal_widget.setEnabled(enable)

    def _on_user_edit(self, text):
        if self.proxy.binding is None:
            return
        if text not in ('', '-'):
            self.proxy.edit_value = int(text, base=16)
        else:
            self.proxy.edit_value = 0
