import os

from PyQt4.QtGui import QLabel, QPixmap
from traits.api import Instance, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import LampModel
from karabo_gui import icons
from karabo_gui.binding.api import (
    BaseBindingController, StringBinding, register_binding_controller
)

state_lamp = {
    State.CHANGING: 'lamp-changing',
    State.ACTIVE: 'lamp-active',
    State.PASSIVE: 'lamp-passive',
    State.STATIC: 'lamp-static',
    State.INIT: 'lamp-init',
    State.NORMAL: 'lamp-known',
    State.KNOWN: 'lamp-known',
    State.ERROR: 'lamp-error',
    State.UNKNOWN: 'lamp-unknown',
    State.DISABLED: 'lamp-disabled'
}


def _is_compatible(binding):
    # XXX: proxy.path == 'state'???
    return False


@register_binding_controller(ui_name='Generic Lamp', read_only=True,
                             binding_type=StringBinding,
                             is_compatible=_is_compatible)
class LampWidget(BaseBindingController):
    model = Instance(LampModel)

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setScaledContents(True)

        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        if State(value).isDerivedFrom(State.CHANGING):
            self._set_lamp(state_lamp[State.CHANGING])
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._set_lamp(state_lamp[State.ACTIVE])
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._set_lamp(state_lamp[State.PASSIVE])
        elif State(value).isDerivedFrom(State.DISABLED):
            self._set_lamp(state_lamp[State.DISABLED])
        elif State(value) is State.STATIC:
            self._set_lamp(state_lamp[State.STATIC])
        elif State(value) is State.NORMAL:
            self._set_lamp(state_lamp[State.NORMAL])
        elif State(value) is State.ERROR:
            self._set_lamp(state_lamp[State.ERROR])
        elif State(value) is State.INIT:
            self._set_lamp(state_lamp[State.INIT])
        else:
            self._set_lamp(state_lamp[State.UNKNOWN])

    def _set_lamp(self, name):
        p = QPixmap(os.path.join(os.path.dirname(icons.__file__), name))
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())
