#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, on_trait_change

from karabo.common.api import KARABO_SCHEMA_REGEX
from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel, HexadecimalModel,
    IntLineEditModel)
from karabogui.binding.api import (
    FloatBinding, IntBinding, StringBinding, get_editor_value)
from karabogui.controllers.api import (
    BaseLineEditController, register_binding_controller)
from karabogui.util import SignalBlocker
from karabogui.validators import (
    HexValidator, IntValidator, NumberValidator, RegexValidator)

MAX_FLOATING_PRECISION = 12


@register_binding_controller(ui_name="Float Field", can_edit=True,
                             klassname="DoubleLineEdit",
                             binding_type=FloatBinding, priority=10)
class DoubleLineEdit(BaseLineEditController):
    model = Instance(DoubleLineEditModel, args=())

    def create_widget(self, parent):
        self.validator = NumberValidator()
        self.validator.decimals = self.model.decimals
        widget = super().create_widget(parent)
        decimal_action = QAction("Change number of decimals", widget)
        decimal_action.triggered.connect(self._pick_decimals)
        widget.addAction(decimal_action)
        return widget

    def value_update(self, proxy):
        value = get_editor_value(proxy, "")
        self.internal_value = str(value)
        if value == "":
            displayed = ""
        else:
            # Note: Avoid numpy to float casting here using the str value
            format_str = ("{}" if self.model.decimals == -1
                          else "{{:.{}f}}".format(self.model.decimals))
            try:
                displayed = format_str.format(float(self.internal_value))
            except ValueError:
                # This can happen for example if a float becomes a
                # vector but controller was not mutated
                displayed = self.internal_value
        with SignalBlocker(self.internal_widget):
            self.display_value = displayed
            self.internal_widget.setText(self.display_value)
        self.internal_widget.setCursorPosition(self.last_cursor_pos)

    @on_trait_change("model.decimals", post_init=True)
    def _decimals_update(self):
        self.value_update(self.proxy)
        self.validator.decimals = self.model.decimals

    def _pick_decimals(self, checked):
        num_decimals, ok = QInputDialog.getInt(
            self.widget, "Decimal", "Floating point precision:",
            self.model.decimals, -1, MAX_FLOATING_PRECISION)
        if ok:
            self.model.decimals = num_decimals

    def validate_value(self):
        ret = super().validate_value()
        return float(ret) if ret is not None else None


@register_binding_controller(ui_name="Integer Field", can_edit=True,
                             klassname="IntLineEdit", binding_type=IntBinding,
                             priority=10)
class IntLineEdit(BaseLineEditController):
    model = Instance(IntLineEditModel, args=())

    def create_widget(self, parent):
        self.validator = IntValidator()
        return super().create_widget(parent)

    def value_update(self, proxy):
        value = get_editor_value(proxy, "")
        self.internal_value = str(value)
        with SignalBlocker(self.internal_widget):
            self.display_value = "{}".format(value)
            self.internal_widget.setText(self.display_value)
        self.internal_widget.setCursorPosition(self.last_cursor_pos)

    def validate_value(self):
        """Reimplemented method of `BaseLineEditController`"""
        ret = super().validate_value()
        return int(ret) if ret is not None else None


@register_binding_controller(ui_name="Hexadecimal", can_edit=True,
                             klassname="Hexadecimal", binding_type=IntBinding)
class Hexadecimal(BaseLineEditController):
    model = Instance(HexadecimalModel, args=())

    def create_widget(self, parent):
        self.validator = HexValidator()
        return super().create_widget(parent)

    def value_update(self, proxy):
        value = get_editor_value(proxy, "")
        if value == "":
            return
        self.internal_value = str(value)
        with SignalBlocker(self.internal_widget):
            self.display_value = "{:x}".format(value)
            self.internal_widget.setText(self.display_value)
        self.internal_widget.setCursorPosition(self.last_cursor_pos)

    def validate_value(self):
        """Reimplemented method of `BaseLineEditController`"""
        ret = super().validate_value()
        return int(ret, base=16) if ret is not None else None


def _is_regex_compatible(binding):
    return binding.attributes.get(KARABO_SCHEMA_REGEX, None) is not None


@register_binding_controller(ui_name="Regex Field", can_edit=True,
                             is_compatible=_is_regex_compatible,
                             klassname="RegexEdit", binding_type=StringBinding,
                             priority=90)
class EditRegex(BaseLineEditController):
    model = Instance(EditableRegexModel, args=())

    def create_widget(self, parent):
        self.validator = RegexValidator()
        return super().create_widget(parent)

    def binding_update(self, proxy):
        binding = proxy.binding
        regex = binding.attributes.get(KARABO_SCHEMA_REGEX, "")
        self.validator.setRegex(regex)
        self.widget.update_unit_label(proxy)

    def value_update(self, proxy):
        value = get_editor_value(proxy, "")
        self.internal_value = str(value)
        with SignalBlocker(self.internal_widget):
            self.display_value = "{}".format(value)
            self.internal_widget.setText(self.display_value)
        self.internal_widget.setCursorPosition(self.last_cursor_pos)

    def validate_value(self):
        """Reimplemented method of `BaseLineEditController`"""
        value = self.internal_value
        state, _, _ = self.validator.validate(value, 0)
        if state == QValidator.Invalid:
            value = None
        return str(value) if value is not None else None
