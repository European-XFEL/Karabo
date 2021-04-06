from qtpy.QtWidgets import QAction
from traits.api import Instance, Str, on_trait_change

from karabo.common.scenemodel.api import DisplayStateColorModel
from karabogui.binding.api import StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.indicators import get_state_color
from karabogui.util import generateObjectName
from karabogui.widgets.hints import FrameWidget


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
        widget = FrameWidget(parent)
        objectName = generateObjectName(self)
        self._style_sheet = ("QLabel#{}".format(objectName) +
                             " {{ background-color : rgb{}; }}")
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
        color = get_state_color(value)
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

    def _show_state_string(self):
        self.model.show_string = not self.model.show_string
