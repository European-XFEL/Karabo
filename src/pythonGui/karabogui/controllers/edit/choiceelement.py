#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 28, 2012
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QComboBox
from traits.api import Instance

from karabo.common.scenemodel.api import EditableChoiceElementModel
from karabogui.binding.api import ChoiceOfNodesBinding
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.util import MouseWheelEventBlocker, SignalBlocker


@register_binding_controller(ui_name='Choice Element', can_edit=True,
                             klassname='EditableChoiceElement',
                             binding_type=ChoiceOfNodesBinding)
class EditableChoiceElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(EditableChoiceElementModel, args=())
    # Internal traits
    _blocker = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)

        self._blocker = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._blocker)
        widget.currentIndexChanged[int].connect(self._on_user_edit)
        widget.setFocusPolicy(Qt.StrongFocus)
        return widget

    def binding_update(self, proxy):
        binding = proxy.binding
        with SignalBlocker(self.widget):
            self.widget.clear()
            for name in binding.choices:
                self.widget.addItem(name)

        # Make sure the correct item is selected
        self.value_update(proxy)

    def value_update(self, proxy):
        index = self.widget.findText(proxy.binding.choice)
        if index >= 0:
            with SignalBlocker(self.widget):
                self.widget.setCurrentIndex(index)

    def _on_user_edit(self, index):
        if self.proxy.binding is None:
            return
        self.proxy.binding.choice = self.widget.itemText(index)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)
