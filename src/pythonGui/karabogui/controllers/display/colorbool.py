#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4.QtCore import QByteArray, pyqtSlot
from PyQt4.QtGui import QAction
from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import ColorBoolModel
from karabogui import icons
from karabogui.binding.api import (
    BaseBindingController, BoolBinding, register_binding_controller)
from karabogui.icons.statefulicons.color_change_icon import (
    ColorChangeIcon, get_color_change_icon)
from karabogui.indicators import STATE_COLORS


@register_binding_controller(ui_name='Switch Bool', read_only=True,
                             binding_type=BoolBinding)
class DisplayColorBool(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ColorBoolModel)
    # A icon which will be used in different colors
    icon = Instance(ColorChangeIcon)

    def _icon_default(self):
        path = op.join(op.dirname(icons.__file__), 'switch-bool.svg')
        return get_color_change_icon(path)

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        widget.setMaximumSize(24, 24)
        widget.resize(20, 20)

        logicAction = QAction("Invert color logic", widget)
        logicAction.triggered.connect(self.logic_action)
        widget.addAction(logicAction)
        return widget

    @on_trait_change('proxy:value,model.invert', post_init=True)
    def _value_update(self):
        binding = self.proxy.binding
        if None in (binding, self.widget):
            # We might get here when the scene model changes and the widget or
            # proxy is not yet ready.
            return

        value = self.proxy.value
        if not self.model.invert:
            color_state = State.ACTIVE if value else State.PASSIVE
        else:
            color_state = State.PASSIVE if value else State.ACTIVE

        svg = self.icon.with_color(STATE_COLORS[color_state])
        self.widget.load(QByteArray(svg))

    @pyqtSlot()
    def logic_action(self):
        self.model.invert = not self.model.invert
