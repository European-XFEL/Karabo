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
from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel, HexadecimalModel,
    IntLineEditModel)
from karabogui.binding.api import (
    FloatBinding, IntBinding, StringBinding, get_min_max)
from karabogui.controllers.api import (
    BaseLineEditController, get_regex, has_regex, register_binding_controller)
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
                      else f"{{:.{self.model.decimals}f}}")
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
        return f"{value:x}"

    def fromString(self, value):
        """Reimplemented method of `BaseLineEditController`"""
        return int(value, base=16)


@register_binding_controller(ui_name="Regex Field", can_edit=True,
                             is_compatible=has_regex,
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
        self.validator.setRegex(get_regex(proxy.binding, ""))
