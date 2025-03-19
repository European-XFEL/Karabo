#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 24, 2019
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
from qtpy.QtGui import QFont
from qtpy.QtWidgets import QFrame, QLabel
from traits.api import Instance

from karabo.common.scenemodel.api import WidgetNodeModel
from karabogui.binding.api import WidgetNodeBinding
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName


@register_binding_controller(ui_name='Widget Node Field',
                             klassname='WidgetNode',
                             binding_type=WidgetNodeBinding,
                             priority=90)
class DisplayWidgetNode(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(WidgetNodeModel, args=())

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setWordWrap(True)
        widget.setAlignment(Qt.AlignCenter)

        objectName = generateObjectName(self)
        widget.setObjectName(objectName)
        style_sheet = (f"QWidget#{objectName}" +
                       f" {{ background-color : rgba{ALL_OK_COLOR}; }}")
        widget.setStyleSheet(style_sheet)
        widget.setFrameStyle(QFrame.Box)
        widget.setFont(QFont("Times", 8, QFont.Cursive))

        return widget

    def binding_update(self, proxy):
        info = proxy.binding.displayType.split('|')
        # The real node type is mangled in the displayType
        node_type = info[1] if len(info) > 1 else "NaN"
        template = f"WidgetNode\nNodeType: {node_type}"
        self.widget.setText(template)
