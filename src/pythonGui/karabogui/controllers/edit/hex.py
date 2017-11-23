#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from numpy import log2
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QLineEdit
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import HexadecimalModel
from karabogui.binding.api import IntBinding, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.controllers.unitlabel import add_unit_label


@register_binding_controller(ui_name='Hexadecimal', can_edit=True,
                             binding_type=IntBinding)
class Hexadecimal(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(HexadecimalModel)
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

    def _widget_changed(self):
        """Init a freshly assigned widget"""
        binding = self.proxy.binding
        if binding is not None:
            self._binding_update(binding)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        low, high = get_min_max(binding)
        num_bits = log2(max(abs(high), abs(low)))
        mask = 'h' * int(num_bits // 4 + 1)
        if low < 0:
            mask = "#" + mask
        self._internal_widget.setInputMask(mask)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.update_label(self.proxy)
        self._internal_widget.setText("{:x}".format(value))

    @pyqtSlot(str)
    def _on_user_edit(self, text):
        if text not in ('', '-'):
            self.proxy.value = int(text, base=16)
        else:
            self.proxy.value = 0
