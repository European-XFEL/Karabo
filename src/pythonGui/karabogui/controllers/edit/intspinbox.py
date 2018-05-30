#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QLocale, Qt
from PyQt4.QtGui import QSpinBox
from traits.api import Instance

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabogui.binding.api import IntBinding, get_editor_value, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, register_binding_controller)
from karabogui.util import MouseWheelEventBlocker, SignalBlocker


class _FocusynSpinBox(QSpinBox):
    """QSpinBox apparently can't be monitored for focus changes with an
    eventFilter. So, we do it the hard way.

    Chemist: 'You can't just go "off" Focusyn!?'
    """
    focusChanged = pyqtSignal(bool)

    def focusInEvent(self, event):
        self.focusChanged.emit(True)
        super(_FocusynSpinBox, self).focusInEvent(event)

    def focusOutEvent(self, event):
        self.focusChanged.emit(False)
        super(_FocusynSpinBox, self).focusOutEvent(event)


@register_binding_controller(ui_name='Integer Spin Box', can_edit=True,
                             klassname='EditableSpinBox',
                             binding_type=IntBinding)
class EditableSpinBox(BaseBindingController):
    # The scene model class for this controller
    model = Instance(EditableSpinBoxModel, args=())
    # Internal details
    _internal_widget = Instance(QSpinBox)
    _blocker = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        self._internal_widget = _FocusynSpinBox(parent)
        self._internal_widget.setLocale(QLocale('en_US'))
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.focusChanged.connect(self._focus_changed)
        return add_unit_label(self.proxy, self._internal_widget, parent=parent)

    def set_read_only(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            if self._blocker is None:
                self._blocker = MouseWheelEventBlocker(self._internal_widget)
            self._internal_widget.installEventFilter(self._blocker)
            self._internal_widget.valueChanged[int].connect(self._on_user_edit)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

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

    @pyqtSlot(bool)
    def _focus_changed(self, has_focus):
        """Make sure edits get applied when focus is lost"""
        if not has_focus:
            value = self._internal_widget.value()
            proxy_value = get_editor_value(self.proxy)
            if value != proxy_value:
                self.proxy.edit_value = value

    @pyqtSlot(int)
    def _on_user_edit(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value
