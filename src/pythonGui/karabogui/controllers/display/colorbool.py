#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 9, 2017
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
import os.path as op

from qtpy.QtWidgets import QAction
from traits.api import Instance, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import ColorBoolModel
from karabogui import icons
from karabogui.binding.api import BoolBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.icons.statefulicons.color_change_icon import (
    ColorChangeIcon, get_color_change_icon)
from karabogui.indicators import STATE_COLORS
from karabogui.widgets.hints import SvgWidget


@register_binding_controller(ui_name='Switch Bool',
                             klassname='DisplayColorBool',
                             binding_type=BoolBinding)
class DisplayColorBool(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(ColorBoolModel, args=())
    # A icon which will be used in different colors
    icon = Instance(ColorChangeIcon)

    def _icon_default(self):
        path = op.join(op.dirname(icons.__file__), 'switch-bool.svg')
        return get_color_change_icon(path)

    def create_widget(self, parent):
        widget = SvgWidget(parent)
        logicAction = QAction("Invert color logic", widget)
        logicAction.triggered.connect(self.logic_action)
        widget.addAction(logicAction)
        # update the context menu and keep track
        logicAction.setCheckable(True)
        logicAction.setChecked(self.model.invert)

        return widget

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if self.widget is None or value is None:
            # We might get here when the scene model changes and the widget
            # is not yet ready or the property proxy is not set.
            return

        if not self.model.invert:
            color_state = State.ACTIVE if value else State.PASSIVE
        else:
            color_state = State.PASSIVE if value else State.ACTIVE
        # Set the tooltip according to the value
        self.widget.setToolTip(f"{value}")
        svg = self.icon.with_color(STATE_COLORS[color_state])
        self.widget.load(bytearray(svg, encoding='UTF-8'))

    @on_trait_change('model.invert')
    def _invert_update(self):
        if self.proxy is not None:
            self.value_update(self.proxy)

    def logic_action(self):
        self.model.invert = not self.model.invert
