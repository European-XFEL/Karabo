#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QComboBox
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import ComboBoxModel
from karabo_gui.binding.api import (
    BaseBindingController, BaseBinding, register_binding_controller)


def _widget_is_compatible(binding):
    return len(binding.options) > 0


@register_binding_controller(ui_name='Combo Box',
                             # XXX: get_compatible_controllers needs to deal
                             # with this special category of widgets later
                             binding_type=BaseBinding.__subclasses__(),
                             is_compatible=_widget_is_compatible,
                             read_only=True)
class DisplayComboBox(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ComboBoxModel)

    def create_widget(self, parent):
        widget = QComboBox(parent)
        widget.setFrame(False)
        widget.setEnabled(False)
        _fill_widget(widget, self.proxy.binding)
        return widget

    @on_trait_change('proxy:binding')
    def _binding_update(self):
        binding = self.proxy.binding
        if binding is None:
            return

        self.widget.clear()
        _fill_widget(self.widget, binding)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        index = self.widget.findText(str(value))
        if index < 0:
            return

        self.widget.setCurrentIndex(index)


def _fill_widget(widget, binding):
    widget.addItems([str(o) for o in binding.options])
