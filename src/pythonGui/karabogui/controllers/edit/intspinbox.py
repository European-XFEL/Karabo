#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QSpinBox
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabogui.binding.api import (
    BaseBindingController, IntBinding, register_binding_controller, get_min_max
)
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.util import MouseWheelEventBlocker


@register_binding_controller(ui_name='Integer Spin Box',
                             binding_type=IntBinding)
class EditableSpinBox(BaseBindingController):
    # The scene model class for this controller
    model = Instance(EditableSpinBoxModel)
    # Internal details
    _internal_widget = Instance(QSpinBox)
    _blocker = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        self._internal_widget = QSpinBox(parent)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.setFocusPolicy(Qt.NoFocus)
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

    def _widget_changed(self):
        """Called after the widget is assigned"""
        binding = self.proxy.binding
        if binding is not None:
            self._binding_update(binding)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        if self.widget is None:
            return

        low, high = get_min_max(binding)
        self._internal_widget.setRange(max(-0x80000000, low),
                                       min(0x7fffffff, high))

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.update_label(self.proxy)
        self._internal_widget.setValue(value)

    @pyqtSlot(int)
    def _on_user_edit(self, value):
        if self.proxy.binding is not None:
            self.proxy.value = value
