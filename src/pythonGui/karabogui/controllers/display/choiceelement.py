#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 6, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtWidgets import QComboBox
from traits.api import Instance

from karabo.common.scenemodel.api import ChoiceElementModel
from karabogui.binding.api import ChoiceOfNodesBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)


@register_binding_controller(ui_name='Choice Element',
                             klassname='DisplayChoiceElement',
                             binding_type=ChoiceOfNodesBinding)
class DisplayChoiceElement(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ChoiceElementModel, args=())

    def binding_update(self, proxy):
        if self.widget is not None:
            self._fill_widget(self.widget, proxy.binding)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)
        self._fill_widget(widget, self.proxy.binding)
        return widget

    def value_update(self, proxy):
        index = self.widget.findText(proxy.binding.choice)
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
