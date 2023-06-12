#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
