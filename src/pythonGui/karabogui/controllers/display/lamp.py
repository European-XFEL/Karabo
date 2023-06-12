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
import os.path as op

from qtpy.QtGui import QPixmap
from qtpy.QtWidgets import QLabel
from traits.api import Instance

from karabo.common.api import State
from karabo.common.scenemodel.api import LampModel
from karabogui import icons
from karabogui.binding.api import StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)

ICONS_DIR = op.dirname(icons.__file__)
STATE_LAMP_PATH = {
    State.CHANGING: op.join(ICONS_DIR, 'lamp-changing'),
    State.RUNNING: op.join(ICONS_DIR, 'lamp-running'),
    State.ACTIVE: op.join(ICONS_DIR, 'lamp-active'),
    State.PASSIVE: op.join(ICONS_DIR, 'lamp-passive'),
    State.STATIC: op.join(ICONS_DIR, 'lamp-static'),
    State.INIT: op.join(ICONS_DIR, 'lamp-init'),
    State.NORMAL: op.join(ICONS_DIR, 'lamp-known'),
    State.KNOWN: op.join(ICONS_DIR, 'lamp-known'),
    State.ERROR: op.join(ICONS_DIR, 'lamp-error'),
    State.UNKNOWN: op.join(ICONS_DIR, 'lamp-unknown'),
    State.DISABLED: op.join(ICONS_DIR, 'lamp-disabled')
}


@register_binding_controller(ui_name='Generic Lamp', klassname='LampWidget',
                             binding_type=StringBinding,
                             is_compatible=with_display_type('State'),
                             can_show_nothing=False)
class LampWidget(BaseBindingController):
    model = Instance(LampModel, args=())

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setScaledContents(True)
        return widget

    def value_update(self, proxy):
        value = proxy.value
        state = State(value)
        if state.isDerivedFrom(State.CHANGING):
            self._set_lamp(STATE_LAMP_PATH[State.CHANGING])
        elif state.isDerivedFrom(State.RUNNING):
            self._set_lamp(STATE_LAMP_PATH[State.RUNNING])
        elif state.isDerivedFrom(State.ACTIVE):
            self._set_lamp(STATE_LAMP_PATH[State.ACTIVE])
        elif state.isDerivedFrom(State.PASSIVE):
            self._set_lamp(STATE_LAMP_PATH[State.PASSIVE])
        elif state.isDerivedFrom(State.DISABLED):
            self._set_lamp(STATE_LAMP_PATH[State.DISABLED])
        elif state is State.STATIC:
            self._set_lamp(STATE_LAMP_PATH[State.STATIC])
        elif state is State.NORMAL:
            self._set_lamp(STATE_LAMP_PATH[State.NORMAL])
        elif state is State.ERROR:
            self._set_lamp(STATE_LAMP_PATH[State.ERROR])
        elif state is State.INIT:
            self._set_lamp(STATE_LAMP_PATH[State.INIT])
        else:
            self._set_lamp(STATE_LAMP_PATH[State.UNKNOWN])

    def _set_lamp(self, path):
        p = QPixmap(path)
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())
