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
from qtpy.QtWidgets import QAction, QDialog, QFrame, QLabel
from traits.api import Instance, String, Tuple, Undefined

from karabo.common.scenemodel.api import build_model_config
from karabogui.binding.api import (
    FloatBinding, get_binding_format, get_binding_value)
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.dialogs.api import (
    AlarmDialog, FormatFmtDialog, FormatLabelDialog)
from karabogui.fonts import get_font_size_from_dpi
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.util import generateObjectName
from karabogui.widgets.hints import Label


class BaseLabelController(BaseBindingController):
    model = Undefined

    bg_color = Tuple(ALL_OK_COLOR)
    internal_widget = Instance(QLabel)
    style_sheet = String
    fmt = String("{}")

    def create_widget(self, parent):
        self.internal_widget = Label(parent)
        widget = add_unit_label(self.proxy, self.internal_widget,
                                parent=parent)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)

        objectName = generateObjectName(self)
        self.style_sheet = (f"QWidget#{objectName}" +
                            " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)
        sheet = self.style_sheet.format(ALL_OK_COLOR)
        widget.setStyleSheet(sheet)

        # Add an action for formatting options
        format_action = QAction("Format field", widget)
        format_action.triggered.connect(self._format_field)
        widget.addAction(format_action)
        self._apply_format(widget)

        return widget

    def clear_widget(self):
        """Clear the internal widget when the device goes offline"""
        self.internal_widget.clear()

    def binding_update(self, proxy):
        self.binding_update_proxy(proxy)
        self.widget.update_unit_label(proxy)

    def value_update(self, proxy):
        binding = proxy.binding
        value = get_binding_value(proxy, "")
        if isinstance(value, str):
            # Early bail out for Long binary data (e.g. image) or
            # if the property is not set (Undefined)
            self.internal_widget.setText(value[:255])
            return

        self.value_update_proxy(proxy, value)
        # Provide some pretty value formatting
        if isinstance(binding, FloatBinding):
            value = float(str(value))

        self.internal_widget.setText(self.toString(value))

    # -----------------------------------------------------------------------
    # Subclassing methods

    def binding_update_proxy(self, proxy):
        """Subclass this method to react on a binding update of `proxy`"""

    def value_update_proxy(self, proxy, value):
        """Subclass this method to react on a value update of `proxy`"""

    def toString(self, value):
        """Subclass this method to convert on a value to string"""
        try:
            value = self.fmt.format(value)
        except Exception:
            value = str(value)
        return value

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
        """The widget is passed as an argument in create_widget as it is not
           yet bound to self.widget then"""
        if widget is None:
            widget = self.widget

        # Apply font formatting
        font = widget.font()
        font.setPointSize(get_font_size_from_dpi(self.model.font_size))
        font.setBold(self.model.font_weight == "bold")
        widget.setFont(font)


# --------------------------------------------------------------------


class FormatMixin:
    """The FormatMixin to provide string formatting for floating points"""
    def create_widget(self, parent):
        widget = super().create_widget(parent)

        fmt_action = QAction("Format value", widget)
        fmt_action.triggered.connect(self._fmt_expression)
        widget.addAction(fmt_action)
        self._create_fmt()

        return widget

    @classmethod
    def initialize_model(cls, proxy, model):
        """Initialize the formatting from the binding of the proxy"""
        super().initialize_model(proxy, model)
        fmt, decimals = get_binding_format(proxy.binding)
        model.trait_set(fmt=fmt, decimals=decimals)

    # -----------------------------------------------------------------------
    # Formatting methods

    def _fmt_expression(self):
        value = get_binding_value(self.proxy)
        dialog = FormatFmtDialog(
            value, fmt=self.model.fmt, decimals=self.model.decimals,
            parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(
                fmt=dialog.fmt, decimals=dialog.decimals)
            self._create_fmt()
            self.value_update(self.proxy)

    def _create_fmt(self):
        """Generate the string formatting according to the model settings"""
        self.fmt = f"{{:.{self.model.decimals}{self.model.fmt}}}"


class AlarmMixin:
    """The AlarmMixin to provide alarm configuration for numbers"""

    def create_widget(self, parent):
        widget = super().create_widget(parent)
        alarm_action = QAction("Configure alarms", widget)
        alarm_action.triggered.connect(self._alarm_dialog)
        widget.addAction(alarm_action)

        return widget

    def value_update_proxy(self, proxy, value):
        model = self.model
        alarm_low = model.alarmLow
        alarm_high = model.alarmHigh
        warn_low = model.warnLow
        warn_high = model.warnHigh
        if ((alarm_low is not Undefined and value < alarm_low) or
                (alarm_high is not Undefined and value > alarm_high)):
            self.bg_color = PROPERTY_ALARM_COLOR
        elif ((warn_low is not Undefined and value < warn_low) or
              (warn_high is not Undefined and value > warn_high)):
            self.bg_color = PROPERTY_WARN_COLOR
        else:
            self.bg_color = ALL_OK_COLOR
        sheet = self.style_sheet.format(self.bg_color)
        self.widget.setStyleSheet(sheet)

    def _alarm_dialog(self):
        binding = self.proxy.binding
        config = build_model_config(self.model)
        dialog = AlarmDialog(binding, config, parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(**dialog.values)
            self.value_update(self.proxy)
