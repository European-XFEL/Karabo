#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLineEdit
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import LineEditModel
from karabogui.binding.api import (
    BaseBindingController, StringBinding, register_binding_controller
)
from karabogui.const import WIDGET_MIN_HEIGHT


# XXX: priority = 10
@register_binding_controller(ui_name='Text Field', read_only=True,
                             binding_type=StringBinding)
class DisplayLineEdit(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(LineEditModel)

    def create_widget(self, parent):
        widget = QLineEdit(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setReadOnly(True)
        widget.setFocusPolicy(Qt.NoFocus)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.setText(value)
