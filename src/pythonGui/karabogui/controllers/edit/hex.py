#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from numpy import log2
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QLineEdit
from traits.api import Instance

from karabo.common.scenemodel.api import HexadecimalModel
from karabogui.binding.api import IntBinding, get_editor_value, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, register_binding_controller)
from karabogui.util import SignalBlocker


@register_binding_controller(ui_name='Hexadecimal', can_edit=True,
                             klassname='Hexadecimal', binding_type=IntBinding)
class Hexadecimal(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(HexadecimalModel, args=())
    # Internal traits
    _internal_widget = Instance(QLineEdit)

    def create_widget(self, parent):
        self._internal_widget = QLineEdit(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        return add_unit_label(self.proxy, self._internal_widget, parent=parent)

    def set_read_only(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.textChanged.connect(self._on_user_edit)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

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

    @pyqtSlot(str)
    def _on_user_edit(self, text):
        if self.proxy.binding is None:
            return
        if text not in ('', '-'):
            self.proxy.edit_value = int(text, base=16)
        else:
            self.proxy.edit_value = 0
