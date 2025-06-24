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
from qtpy.QtWidgets import QAction, QDialog, QInputDialog
from traits.api import Instance, String, on_trait_change

from karabo.common.scenemodel.api import FloatSpinBoxModel
from karabogui.binding.api import FloatBinding, get_editor_value, get_min_max
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.dialogs.api import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.util import (
    MouseWheelEventBlocker, SignalBlocker, generateObjectName)
from karabogui.widgets.api import DoubleSpinBox


@register_binding_controller(ui_name="Double SpinBox", can_edit=True,
                             klassname="FloatSpinBox",
                             binding_type=FloatBinding)
class FloatSpinBox(BaseBindingController):
    model = Instance(FloatSpinBoxModel, args=())
    _blocker = Instance(MouseWheelEventBlocker)
    _style_sheet = String

    def create_widget(self, parent):
        widget = DoubleSpinBox(parent)
        widget.setDecimals(self.model.decimals)
        widget.setSingleStep(self.model.step)
        widget.valueChanged[float].connect(self._on_user_edit)
        self._blocker = MouseWheelEventBlocker(widget)
        widget.installEventFilter(self._blocker)

        objectName = generateObjectName(self)
        widget.setObjectName(objectName)

        self._style_sheet = (f"QDoubleSpinBox#{objectName}" +
                             " {{ font: {}; font-size: {}pt; }}")
        # add actions
        step_action = QAction("Change Step...", widget)
        step_action.triggered.connect(self._change_step)
        widget.addAction(step_action)

        decimal_action = QAction("Change Decimals...", widget)
        decimal_action.triggered.connect(self._change_decimals)
        widget.addAction(decimal_action)

        format_action = QAction("Format field...", widget)
        format_action.triggered.connect(self._format_field)
        widget.addAction(format_action)

        # Apply initial formats
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
            self.widget.setRange(low, high)

    def value_update(self, proxy):
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self.widget):
                self.widget.setValue(value)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)

    @on_trait_change("model:decimals", post_init=True)
    def _set_decimals(self, value):
        if self.widget is not None:
            self.widget.setDecimals(value)

    @on_trait_change("model:step", post_init=True)
    def _set_step(self, value):
        if self.widget is not None:
            self.widget.setSingleStep(value)

    def _change_step(self):
        step = self.widget.singleStep()
        step, ok = QInputDialog.getDouble(
            self.widget, "Single Step", "Enter size of a single step", step)
        if ok:
            self.model.step = step

    def _change_decimals(self):
        # Override the starting value with something from the binding
        decimals = self.model.decimals
        decimals, ok = QInputDialog.getInt(
            self.widget, "Decimals", "Enter number of decimals",
            value=decimals, min=0, max=15)
        if ok:
            self.model.decimals = decimals

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
