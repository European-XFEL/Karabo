#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import QLocale, Qt
from qtpy.QtWidgets import QAction, QDialog, QSpinBox
from traits.api import Instance

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabogui.binding.api import IntBinding, get_editor_value, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, is_proxy_allowed,
    register_binding_controller)
from karabogui.dialogs.api import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.util import MouseWheelEventBlocker, SignalBlocker

LOCALE = QLocale("en_US")


@register_binding_controller(ui_name="Integer SpinBox", can_edit=True,
                             klassname="EditableSpinBox",
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

        # Formats
        format_action = QAction("Format field...", widget)
        format_action.triggered.connect(self._format_field)
        widget.addAction(format_action)
        self._apply_format(self._internal_widget)

        return widget

    def binding_update(self, proxy):
        self.widget.update_unit_label(proxy)
        low, high = get_min_max(proxy.binding)
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setRange(max(-0x80000000, low),
                                           min(0x7fffffff, high))

    def value_update(self, proxy):
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

    # -----------------------------------------------------------------------
    # Formatting methods

    def _format_field(self):
        dialog = FormatLabelDialog(font_size=self.model.font_size,
                                   font_weight=self.model.font_weight,
                                   parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(font_size=dialog.font_size,
                                 font_weight=dialog.font_weight)
            self._apply_format()

    def _apply_format(self, widget=None):
        if widget is None:
            widget = self._internal_widget

        # Apply font formatting
        font = widget.font()
        font.setPointSize(get_font_size_from_dpi(self.model.font_size))
        font.setBold(self.model.font_weight == "bold")
        widget.setFont(font)
