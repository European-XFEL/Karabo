#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 6, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QComboBox
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import ChoiceElementModel
from karabogui.binding.api import (
    BaseBindingController, ChoiceOfNodesBinding, register_binding_controller
)


@register_binding_controller(ui_name='Choice Element', read_only=True,
                             binding_type=ChoiceOfNodesBinding)
class DisplayChoiceElement(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ChoiceElementModel)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)
        self._fill_widget(widget, self.proxy.binding)
        return widget

    @on_trait_change('proxy.binding')
    def _binding_update(self, binding):
        if self.widget is not None:
            self._fill_widget(self.widget, binding)

    @on_trait_change('proxy.binding.choice')
    def _value_update(self, obj, name, choice):
        if name != 'choice':
            return

        index = self.widget.findText(choice)
        if index >= 0:
            self.widget.setCurrentIndex(index)

    # -------------------------------------------------------------------------
    # Private

    def _fill_widget(self, widget, binding):
        if binding is None:
            return

        widget.clear()
        for name in binding.choices:
            widget.addItem(name)
