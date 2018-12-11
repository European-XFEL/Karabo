#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 24, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QFont, QFrame, QLabel

from traits.api import Instance

from karabo.common.scenemodel.api import WidgetNodeModel
from karabogui.binding.api import WidgetNodeBinding
from karabogui.const import (
    ALL_OK_COLOR, WIDGET_MIN_HEIGHT)
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
        # The real widget type is mangled in the display_type
        version = info[1] if len(info) > 1 else "NaN"
        template = "WidgetNode\nVersion: {}".format(version)
        self.widget.setText(template)
