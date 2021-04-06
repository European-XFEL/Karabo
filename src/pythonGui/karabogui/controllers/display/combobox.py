#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtWidgets import QComboBox
from traits.api import Instance

from karabo.common.scenemodel.api import ComboBoxModel
from karabogui.binding.api import BaseBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)


def _widget_is_compatible(binding):
    return len(binding.options) > 0


@register_binding_controller(ui_name='Combo Box', klassname='DisplayComboBox',
                             binding_type=BaseBinding,
                             is_compatible=_widget_is_compatible)
class DisplayComboBox(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ComboBoxModel, args=())

    def binding_update(self, proxy):
        self.widget.clear()
        _fill_widget(self.widget, proxy.binding)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)
        widget.setEnabled(False)
        _fill_widget(widget, self.proxy.binding)
        return widget

    def value_update(self, proxy):
        index = self.widget.findText(str(proxy.value))
        if index < 0:
            return

        self.widget.setCurrentIndex(index)


def _fill_widget(widget, binding):
    widget.addItems([str(o) for o in binding.options])
