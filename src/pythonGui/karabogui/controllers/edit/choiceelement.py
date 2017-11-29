#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 28, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QComboBox
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import ChoiceElementModel
from karabogui.binding.api import ChoiceOfNodesBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.util import MouseWheelEventBlocker


@register_binding_controller(ui_name='Choice Element', can_edit=True,
                             klassname='EditableChoiceElement',
                             binding_type=ChoiceOfNodesBinding)
class EditableChoiceElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(ChoiceElementModel)
    # Internal traits
    _blocker = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)

        self._blocker = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._blocker)
        widget.currentIndexChanged[int].connect(self._on_user_edit)
        return widget

    def _widget_changed(self):
        """Finish initialization of the widget if the binding exists"""
        binding = self.proxy.binding
        if binding is not None:
            self._binding_update(binding)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        if self.widget is None:
            return

        self.widget.clear()
        for name in binding.choices:
            self.widget.addItem(name)

    @on_trait_change('proxy.binding.choice')
    def _value_update(self, obj, name, choice):
        if name != 'choice':
            return

        index = self.widget.findText(choice)
        if index >= 0:
            self.widget.setCurrentIndex(index)

    @pyqtSlot(int)
    def _on_user_edit(self, index):
        self.proxy.binding.choice = self.widget.itemText(index)
