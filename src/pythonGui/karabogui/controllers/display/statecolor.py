from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import QAction, QFrame, QLabel
from traits.api import Instance, Str, on_trait_change

from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayStateColorModel
from karabogui.binding.api import StringBinding
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.indicators import STATE_COLORS
from karabogui.util import generateObjectName


@register_binding_controller(ui_name='State Color Field',
                             klassname='DisplayStateColor',
                             binding_type=StringBinding,
                             priority=20,
                             is_compatible=with_display_type('State'),
                             can_show_nothing=False)
class DisplayStateColor(BaseBindingController):
    # The specific scene model class used by this widget
    model = Instance(DisplayStateColorModel, args=())
    # CSS template for the widget BG color
    _style_sheet = Str

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setAutoFillBackground(True)
        widget.setAlignment(Qt.AlignCenter)
        widget.setMinimumWidth(32)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setWordWrap(True)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)

        objectName = generateObjectName(self)
        self._style_sheet = ("QLabel#{}".format(objectName) +
                             " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)

        textAction = QAction("Show State String", widget)
        textAction.triggered.connect(self._show_state_string)
        # update the context menu and keep track
        textAction.setCheckable(True)
        textAction.setChecked(self.model.show_string)
        widget.addAction(textAction)
        return widget

    def value_update(self, proxy):
        value = proxy.value
        if State(value).isDerivedFrom(State.CHANGING):
            color = STATE_COLORS[State.CHANGING]
        elif State(value).isDerivedFrom(State.RUNNING):
            color = STATE_COLORS[State.RUNNING]
        elif State(value).isDerivedFrom(State.ACTIVE):
            color = STATE_COLORS[State.ACTIVE]
        elif State(value).isDerivedFrom(State.PASSIVE):
            color = STATE_COLORS[State.PASSIVE]
        elif State(value).isDerivedFrom(State.DISABLED):
            color = STATE_COLORS[State.DISABLED]
        elif State(value) is State.STATIC:
            color = STATE_COLORS[State.STATIC]
        elif State(value) is State.NORMAL:
            color = STATE_COLORS[State.NORMAL]
        elif State(value) is State.ERROR:
            color = STATE_COLORS[State.ERROR]
        elif State(value) is State.INIT:
            color = STATE_COLORS[State.INIT]
        else:
            color = STATE_COLORS[State.UNKNOWN]

        sheet = self._style_sheet.format(color)
        self.widget.setStyleSheet(sheet)

        if self.model.show_string:
            self.widget.setText(proxy.value)

    @on_trait_change('model.show_string', post_init=True)
    def _update_text(self):
        if self.proxy is not None:
            self.value_update(self.proxy)
            if not self.model.show_string:
                # Only clear the widget once if no action is set!
                self.widget.clear()

    @pyqtSlot()
    def _show_state_string(self):
        self.model.show_string = not self.model.show_string
