#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QComboBox
from traits.api import Instance, Undefined

from karabogui import globals as krb_globals
from karabo.common.scenemodel.api import ComboBoxModel
from karabogui.binding.api import BaseBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import MouseWheelEventBlocker, SignalBlocker


def _is_compatible(binding):
    return len(binding.options) > 0


@register_binding_controller(ui_name='Selection Field', can_edit=True,
                             klassname='EditableComboBox',
                             binding_type=BaseBinding,
                             is_compatible=_is_compatible, priority=20)
class EditableComboBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(ComboBoxModel, args=())
    # Internal traits
    _filter = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)

        self._filter = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._filter)
        widget.currentIndexChanged[int].connect(self._on_user_edit)
        return widget

    def binding_update(self, proxy):
        with SignalBlocker(self.widget):
            self.widget.clear()
            self.widget.addItems([str(o) for o in proxy.binding.options])

    def value_update(self, proxy):
        value = get_editor_value(proxy)
        if value is None:
            return
        options = proxy.binding.options
        try:
            index = next(i for i, v in enumerate(options)
                         if v == value)
            with SignalBlocker(self.widget):
                self.widget.setCurrentIndex(index)
        except StopIteration:
            return

    @pyqtSlot(int)
    def _on_user_edit(self, index):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = self.proxy.binding.options[index]

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
