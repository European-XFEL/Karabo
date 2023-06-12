#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from qtpy.QtWidgets import QAction, QDialog
from traits.api import Instance, String

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabogui.binding.api import (
    Int8Binding, Int16Binding, Int32Binding, Uint8Binding, Uint16Binding,
    Uint32Binding, get_editor_value, get_min_max)
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.dialogs.api import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.util import (
    MouseWheelEventBlocker, SignalBlocker, generateObjectName)
from karabogui.widgets.api import SpinBox

INT_BINDINGS = (Int8Binding, Int16Binding, Int32Binding, Uint8Binding,
                Uint16Binding, Uint32Binding)


@register_binding_controller(ui_name="Integer SpinBox", can_edit=True,
                             klassname="EditableSpinBox",
                             binding_type=INT_BINDINGS)
class EditableSpinBox(BaseBindingController):
    model = Instance(EditableSpinBoxModel, args=())
    _blocker = Instance(MouseWheelEventBlocker)
    _style_sheet = String

    def create_widget(self, parent):
        widget = SpinBox(parent)
        widget.valueChanged[int].connect(self._on_user_edit)
        widget.setSingleStep(1)
        self._blocker = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._blocker)

        objectName = generateObjectName(self)
        widget.setObjectName(objectName)

        self._style_sheet = (f"SpinBox#{objectName}" +
                             " {{ font: {}; font-size: {}pt; }}")

        # Formats
        format_action = QAction("Format field...", widget)
        format_action.triggered.connect(self._format_field)
        widget.addAction(format_action)

        self._apply_format(widget)

        return widget

    def binding_update(self, proxy):
        binding = proxy.binding
        label = None
        if binding is not None:
            label = f" {binding.unit_label}" if binding.unit_label else None
        low, high = get_min_max(binding)
        with SignalBlocker(self.widget):
            self.widget.setSuffix(label)
            self.widget.setRange(max(-0x80000000, low),
                                 min(0x7fffffff, high))

    def value_update(self, proxy):
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self.widget):
                self.widget.setValue(value)

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
            widget = self.widget

        # Apply font formatting
        sheet = self._style_sheet.format(
            self.model.font_weight,
            get_font_size_from_dpi(self.model.font_size))
        widget.setStyleSheet(sheet)
        widget.update()
