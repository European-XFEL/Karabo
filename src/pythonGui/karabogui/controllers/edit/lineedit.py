#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, on_trait_change

from karabo.common.api import KARABO_SCHEMA_REGEX
from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel, HexadecimalModel,
    IntLineEditModel)
from karabogui.binding.api import (
    FloatBinding, IntBinding, StringBinding, get_min_max)
from karabogui.controllers.api import (
    BaseLineEditController, register_binding_controller)
from karabogui.validators import (
    HexValidator, IntValidator, NumberValidator, RegexValidator)

MAX_FLOATING_PRECISION = 12


@register_binding_controller(ui_name="Float Field", can_edit=True,
                             klassname="DoubleLineEdit",
                             binding_type=FloatBinding, priority=10)
class DoubleLineEdit(BaseLineEditController):
    model = Instance(DoubleLineEditModel, args=())

    def create_widget(self, parent):
        widget = super().create_widget(parent)
        decimal_action = QAction("Change number of decimals", widget)
        decimal_action.triggered.connect(self._pick_decimals)
        widget.addAction(decimal_action)
        return widget

    # ----------------------------------------------------------------------
    # Abstract Interface

    def create_validator(self):
        """Reimplemented function of `BaseLineEditController`"""
        return NumberValidator(self.model.decimals)

    def binding_validator(self, proxy):
        """Reimplemented method of `BaseLineEditController`"""
        low, high = get_min_max(proxy.binding)
        self.validator.setBottom(low)
        self.validator.setTop(high)

    def toString(self, value):
        """Reimplemented method of `BaseLineEditController`"""
        format_str = ("{}" if self.model.decimals == -1
                      else "{{:.{}f}}".format(self.model.decimals))
        return format_str.format(float(str(value)))

    def fromString(self, value):
        """Reimplemented method of `BaseLineEditController`"""
        return float(value)

    # ----------------------------------------------------------------------

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


@register_binding_controller(ui_name="Integer Field", can_edit=True,
                             klassname="IntLineEdit", binding_type=IntBinding,
                             priority=10)
class IntLineEdit(BaseLineEditController):
    model = Instance(IntLineEditModel, args=())

    # ----------------------------------------------------------------------
    # Abstract Interface

    def create_validator(self):
        """Reimplemented method of `BaseLineEditController`"""
        return IntValidator()

    def binding_validator(self, proxy):
        """Reimplemented method of `BaseLineEditController`"""
        low, high = get_min_max(proxy.binding)
        self.validator.setBottom(low)
        self.validator.setTop(high)

    def fromString(self, value):
        """Reimplemented method of `BaseLineEditController`"""
        return int(value)


@register_binding_controller(ui_name="Hexadecimal", can_edit=True,
                             klassname="Hexadecimal", binding_type=IntBinding)
class Hexadecimal(BaseLineEditController):
    model = Instance(HexadecimalModel, args=())

    # ----------------------------------------------------------------------
    # Abstract Interface

    def create_validator(self):
        """Reimplemented method of `BaseLineEditController`"""
        return HexValidator()

    def binding_validator(self, proxy):
        """Reimplemented method of `BaseLineEditController`"""
        low, high = get_min_max(proxy.binding)
        self.validator.setBottom(low)
        self.validator.setTop(high)

    def toString(self, value):
        """Reimplemented method of `BaseLineEditController`"""
        return "{:x}".format(value)

    def fromString(self, value):
        """Reimplemented method of `BaseLineEditController`"""
        return int(value, base=16)


def _is_regex_compatible(binding):
    return binding.attributes.get(KARABO_SCHEMA_REGEX, None) is not None


@register_binding_controller(ui_name="Regex Field", can_edit=True,
                             is_compatible=_is_regex_compatible,
                             klassname="RegexEdit", binding_type=StringBinding,
                             priority=90)
class EditRegex(BaseLineEditController):
    model = Instance(EditableRegexModel, args=())

    # ----------------------------------------------------------------------
    # Abstract Interface

    def create_validator(self):
        """Reimplemented method of `BaseLineEditController`"""
        return RegexValidator()

    def binding_validator(self, proxy):
        """Reimplemented method of `BaseLineEditController`"""
        regex = proxy.binding.attributes.get(KARABO_SCHEMA_REGEX, "")
        self.validator.setRegex(regex)
