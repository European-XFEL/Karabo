from qtpy.QtCore import Qt
from qtpy.QtWidgets import QFrame
from traits.api import Instance

from karabo.common.scenemodel.api import DisplayListModel
from karabogui.binding.api import (
    VectorBinding, VectorCharBinding, VectorHashBinding, get_binding_value)
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.indicators import ALL_OK_COLOR
from karabogui.util import generateObjectName
from karabogui.widgets.hints import Label


def _is_compatible(binding):
    return not isinstance(binding, (VectorCharBinding, VectorHashBinding))


@register_binding_controller(ui_name="List", is_compatible=_is_compatible,
                             klassname="DisplayList", priority=10,
                             binding_type=VectorBinding)
class DisplayList(BaseBindingController):
    model = Instance(DisplayListModel, args=())

    def create_widget(self, parent):
        widget = Label(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)
        widget.setFocusPolicy(Qt.NoFocus)
        # XXX: Next step is elided text for the controller
        widget.setWordWrap(True)
        objectName = generateObjectName(self)
        style_sheet = ("QWidget#{}".format(objectName) +
                       " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)
        sheet = style_sheet.format(ALL_OK_COLOR)
        widget.setStyleSheet(sheet)

        return widget

    def value_update(self, proxy):
        value = get_binding_value(proxy, [])
        self.widget.setText(",".join(str(v) for v in value))
