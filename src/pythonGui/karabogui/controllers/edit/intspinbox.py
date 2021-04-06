#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import QLocale, Qt
from qtpy.QtWidgets import QSpinBox
from traits.api import Instance

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabogui.binding.api import IntBinding, get_editor_value, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    add_unit_label, BaseBindingController, is_proxy_allowed,
    register_binding_controller)
from karabogui.util import MouseWheelEventBlocker, SignalBlocker

LOCALE = QLocale('en_US')


@register_binding_controller(ui_name='Integer SpinBox', can_edit=True,
                             klassname='EditableSpinBox',
                             binding_type=IntBinding)
class EditableSpinBox(BaseBindingController):
    model = Instance(EditableSpinBoxModel, args=())

    _internal_widget = Instance(QSpinBox)
    _blocker = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        self._internal_widget = QSpinBox(parent)
        self._internal_widget.setLocale(LOCALE)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.valueChanged[int].connect(self._on_user_edit)
        self._internal_widget.setFocusPolicy(Qt.StrongFocus)
        self._blocker = MouseWheelEventBlocker(self._internal_widget)
        self._internal_widget.installEventFilter(self._blocker)

        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)
        widget.setFocusProxy(self._internal_widget)

        return widget

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        self._internal_widget.setRange(max(-0x80000000, low),
                                       min(0x7fffffff, high))

    def value_update(self, proxy):
        self.widget.update_label(proxy)
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self._internal_widget):
                self._internal_widget.setValue(value)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)

    def _on_user_edit(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value
