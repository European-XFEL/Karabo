#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLineEdit
from traits.api import Instance

from karabo.common.scenemodel.api import LineEditModel
from karabogui.binding.api import StringBinding
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller


@register_binding_controller(ui_name='Text Field', klassname='DisplayLineEdit',
                             binding_type=StringBinding, priority=10)
class DisplayLineEdit(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(LineEditModel)

    def create_widget(self, parent):
        widget = QLineEdit(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setReadOnly(True)
        widget.setFocusPolicy(Qt.NoFocus)
        return widget

    def value_update(self, proxy):
        self.widget.setText(proxy.value)
