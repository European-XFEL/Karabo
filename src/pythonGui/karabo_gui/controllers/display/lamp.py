import os.path as op

from PyQt4.QtGui import QLabel, QPixmap
from traits.api import Instance, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import LampModel
from karabo_gui import icons
from karabo_gui.binding.api import (
    BaseBindingController, StringBinding, register_binding_controller
)
from karabo_gui.controllers.util import with_display_type

ICONS_DIR = op.dirname(icons.__file__)
STATE_LAMP_PATH = {
    State.CHANGING: op.join(ICONS_DIR, 'lamp-changing'),
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


@register_binding_controller(ui_name='Generic Lamp', read_only=True,
                             binding_type=StringBinding,
                             is_compatible=with_display_type('State'))
class LampWidget(BaseBindingController):
    model = Instance(LampModel)

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setScaledContents(True)

        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        if State(value).isDerivedFrom(State.CHANGING):
            self._set_lamp(STATE_LAMP_PATH[State.CHANGING])
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._set_lamp(STATE_LAMP_PATH[State.ACTIVE])
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._set_lamp(STATE_LAMP_PATH[State.PASSIVE])
        elif State(value).isDerivedFrom(State.DISABLED):
            self._set_lamp(STATE_LAMP_PATH[State.DISABLED])
        elif State(value) is State.STATIC:
            self._set_lamp(STATE_LAMP_PATH[State.STATIC])
        elif State(value) is State.NORMAL:
            self._set_lamp(STATE_LAMP_PATH[State.NORMAL])
        elif State(value) is State.ERROR:
            self._set_lamp(STATE_LAMP_PATH[State.ERROR])
        elif State(value) is State.INIT:
            self._set_lamp(STATE_LAMP_PATH[State.INIT])
        else:
            self._set_lamp(STATE_LAMP_PATH[State.UNKNOWN])

    def _set_lamp(self, path):
        p = QPixmap(path)
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())
