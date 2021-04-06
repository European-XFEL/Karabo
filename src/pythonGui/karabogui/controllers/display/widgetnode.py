#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 24, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import Qt
from qtpy.QtGui import QFont
from qtpy.QtWidgets import QFrame, QLabel

from traits.api import Instance

from karabo.common.scenemodel.api import WidgetNodeModel
from karabogui.binding.api import WidgetNodeBinding
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.indicators import ALL_OK_COLOR
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
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
        style = ("QWidget#{}".format(objectName) +
                 " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)
        sheet = style.format(ALL_OK_COLOR)
        widget.setStyleSheet(sheet)
        widget.setFrameStyle(QFrame.Box)
        widget.setFont(QFont("Times", 8, QFont.Cursive))

        return widget

    def binding_update(self, proxy):
        info = proxy.binding.display_type.split('|')
        # The real node type is mangled in the display_type
        node_type = info[1] if len(info) > 1 else "NaN"
        template = "WidgetNode\nNodeType: {}".format(node_type)
        self.widget.setText(template)
