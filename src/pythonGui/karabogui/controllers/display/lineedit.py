#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QLineEdit
from traits.api import Instance

from karabo.common.scenemodel.api import LineEditModel
from karabogui.binding.api import StringBinding, get_binding_value
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)


@register_binding_controller(ui_name='Text Field', klassname='DisplayLineEdit',
                             binding_type=StringBinding, priority=10)
class DisplayLineEdit(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(LineEditModel, args=())

    def create_widget(self, parent):
        widget = QLineEdit(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setReadOnly(True)
        widget.setFocusPolicy(Qt.NoFocus)
        return widget

    def value_update(self, proxy):
        self.widget.setText(get_binding_value(proxy, ''))
