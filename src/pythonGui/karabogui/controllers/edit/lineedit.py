#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import QSize, Qt
from PyQt5.QtGui import QFontMetrics
from PyQt5.QtWidgets import QLineEdit
from traits.api import Instance, Int

from karabo.common.scenemodel.api import LineEditModel
from karabogui.binding.api import (
    CharBinding, StringBinding, VectorCharBinding, get_editor_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.const import WIDGET_MIN_HEIGHT, WIDGET_MIN_WIDTH
from karabogui.util import SignalBlocker

BINDING_TYPES = (CharBinding, StringBinding, VectorCharBinding)


class LineEdit(QLineEdit):
    def __init__(self, parent):
        super(LineEdit, self).__init__(parent)
        self.setMinimumWidth(WIDGET_MIN_WIDTH)
        self.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.setAlignment(Qt.AlignLeft | Qt.AlignAbsolute)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        CONTENT_MARGIN = 10
        width = fm.width(self.text()) + CONTENT_MARGIN

        return QSize(width, 20)


@register_binding_controller(ui_name='Text Field', can_edit=True,
                             klassname='EditableLineEdit', priority=10,
                             binding_type=BINDING_TYPES)
class EditableLineEdit(BaseBindingController):
    model = Instance(LineEditModel, args=())
    # Internal details
    _last_cursor_pos = Int(0)

    def create_widget(self, parent):
        widget = LineEdit(parent)
        widget.textChanged.connect(self._on_text_changed)
        return widget

    def binding_update(self, proxy):
        if isinstance(proxy.binding, CharBinding):
            mask = 'X'
            with SignalBlocker(self.widget):
                self.widget.setInputMask(mask)

    def value_update(self, proxy):
        value = get_editor_value(proxy, '')
        if not isinstance(value, str):
            value = value.decode()

        with SignalBlocker(self.widget):
            self.widget.setText(value)
        self.widget.setCursorPosition(self._last_cursor_pos)

    def _on_text_changed(self, value):
        if self.proxy.binding is None:
            return
        self._last_cursor_pos = self.widget.cursorPosition()
        self.proxy.edit_value = value
