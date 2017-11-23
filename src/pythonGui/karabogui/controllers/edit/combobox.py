#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QComboBox
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import ComboBoxModel
from karabogui.binding.api import BaseBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.util import MouseWheelEventBlocker


def _is_compatible(binding):
    return len(binding.options) > 0


# XXX: priority = 20
@register_binding_controller(ui_name='Selection Field', can_edit=True,
                             binding_type=BaseBinding,
                             is_compatible=_is_compatible)
class EditableComboBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(ComboBoxModel)
    # Internal traits
    _filter = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)

        self._filter = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._filter)
        widget.currentIndexChanged[int].connect(self._on_user_edit)
        return widget

    def _widget_changed(self):
        """The widget was just initialized"""
        binding = self.proxy.binding
        if binding is not None:
            self._binding_update(binding)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        if self.widget is None:
            return

        self.widget.clear()
        self.widget.addItems([str(o) for o in binding.options])

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        options = self.proxy.binding.options
        try:
            index = next(i for i, v in enumerate(options)
                         if v == value)
            self.widget.setCurrentIndex(index)
        except StopIteration:
            return

    @pyqtSlot(int)
    def _on_user_edit(self, index):
        if self.proxy.binding is not None:
            self.proxy.value = self.proxy.binding.options[index]
