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
from qtpy.QtCore import Qt
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QLineEdit
from traits.api import Instance, Int, String, Undefined

from karabogui.binding.api import get_editor_value
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.controllers.util import is_proxy_allowed
from karabogui.util import SignalBlocker, generateObjectName
from karabogui.widgets.hints import LineEdit

FINE_COLOR = "black"
ERROR_COLOR = "red"


class BaseLineEditController(BaseBindingController):
    """The `BaseLineEditController` class provides a visual indication
    if a set text provides acceptable input validated by a subclassed
     `validator` instance.
    """
    model = Undefined
    # The line edit widget instance
    internal_widget = Instance(QLineEdit)
    # The validator instance
    validator = Instance(QValidator, allow_none=True)
    # The last cursor position
    last_cursor_pos = Int(0)

    # Private properties
    _style_sheet = String("")

    def create_widget(self, parent):
        self.internal_widget = LineEdit(parent)
        self.validator = self.create_validator()
        self.internal_widget.setValidator(self.validator)
        objectName = generateObjectName(self)
        self._style_sheet = (f"QWidget#{objectName}" +
                             " {{ color: {}; }}")
        self.internal_widget.setObjectName(objectName)
        sheet = self._style_sheet.format(FINE_COLOR)
        self.internal_widget.setStyleSheet(sheet)
        widget = add_unit_label(self.proxy, self.internal_widget,
                                parent=parent)
        widget.setFocusProxy(self.internal_widget)
        return widget

    def set_read_only(self, ro):
        self.internal_widget.setReadOnly(ro)
        if not ro:
            self.internal_widget.textChanged.connect(self._on_text_changed)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self.internal_widget.setFocusPolicy(focus_policy)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.internal_widget.setEnabled(enable)

    def setEnabled(self, enable):
        self.internal_widget.setEnabled(enable)

    def binding_update(self, proxy):
        self.binding_validator(proxy)
        self.widget.update_unit_label(proxy)

    def value_update(self, proxy):
        value = get_editor_value(proxy, "")
        if value != "":
            try:
                value = self.toString(value)
            except Exception:
                value = str(value)
        with SignalBlocker(self.internal_widget):
            self.internal_widget.setText(value)
        self.internal_widget.setCursorPosition(self.last_cursor_pos)

    def on_decline(self):
        """When the input was declined, this method is executed"""
        sheet = self._style_sheet.format(FINE_COLOR)
        self.internal_widget.setStyleSheet(sheet)

    # Private interface
    # ---------------------------------------------------------------------

    def _on_text_changed(self, text):
        if self.proxy.binding is None:
            sheet = self._style_sheet.format(FINE_COLOR)
            self.internal_widget.setStyleSheet(sheet)
            return

        self.onText(text)

    # Public interface
    # ---------------------------------------------------------------------

    def validate_text_color(self):
        """Public method to validate the text color"""
        acceptable_input = self.internal_widget.hasAcceptableInput()
        color = FINE_COLOR if acceptable_input else ERROR_COLOR
        sheet = self._style_sheet.format(color)
        self.internal_widget.setStyleSheet(sheet)

    # Abstract interface
    # ----------------------------------------------------------------------

    def onText(self, text):
        """Subclass method to react on the update of the internal widget"""
        color = FINE_COLOR
        acceptable_input = self.internal_widget.hasAcceptableInput()
        if acceptable_input:
            self.last_cursor_pos = self.internal_widget.cursorPosition()
            self.proxy.edit_value = self.fromString(text)
        else:
            color = ERROR_COLOR
            # Erase the edit value by setting it to `None`
            self.proxy.edit_value = None
        sheet = self._style_sheet.format(color)
        self.internal_widget.setStyleSheet(sheet)

    def create_validator(self):
        """Subclass method to specify a validator instance of `QValidator`"""

    def binding_validator(self, proxy):
        """Subclass method to update a validator on binding update"""

    def toString(self, value):
        """Subclass method to convert a value to string"""
        return str(value)

    def fromString(self, value):
        """Subclass method to convert a value from a string"""
        return value
