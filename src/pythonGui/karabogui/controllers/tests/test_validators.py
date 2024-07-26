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
from qtpy.QtGui import QValidator

from karabo.common.const import KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MIN_INC
from karabogui.binding.api import (
    FloatBinding, Uint8Binding, VectorFloatBinding, VectorUint8Binding)

from ..validators import BindingValidator, ListValidator, SimpleValidator


def test_generic_binding_validator():
    binding = FloatBinding()
    validator = BindingValidator(binding=binding)
    result, _, _ = validator.validate("", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("2.12", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("2....", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("2.", None)
    assert result == QValidator.Acceptable
    # A very generic validator case. Can still cast with traits ...
    result, _, _ = validator.validate(" 2.", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate(" ", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("2 ", None)
    assert result == QValidator.Acceptable

    binding = VectorFloatBinding()
    validator = BindingValidator(binding=binding)
    result, _, _ = validator.validate([], None)
    assert result == QValidator.Acceptable
    # float and integer
    result, _, _ = validator.validate([2.2, 1], None)
    assert result == QValidator.Acceptable
    # float and string that can be casted.
    result, _, _ = validator.validate([2, "2"], None)
    assert result == QValidator.Acceptable
    # Even bools are casted by traits ...
    result, _, _ = validator.validate([True, False], None)
    assert result == QValidator.Acceptable


def test_list_binding_unsigned_validator():
    binding = VectorUint8Binding()
    validator = ListValidator(binding=binding)
    result, _, _ = validator.validate("", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("1", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("0", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("0,2,1,12,45,12", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("+", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("-", None)
    assert result == QValidator.Invalid

    # But not boolean
    result, _, _ = validator.validate("Fals", None)
    assert result == QValidator.Invalid
    # Floating input
    result, _, _ = validator.validate("0.", None)
    assert result == QValidator.Invalid

    result, _, _ = validator.validate("0.0", None)
    assert result == QValidator.Invalid

    # Sign at the end
    result, _, _ = validator.validate("0-", None)
    assert result == QValidator.Invalid

    result, _, _ = validator.validate("0.0-", None)
    assert result == QValidator.Invalid

    result, _, _ = validator.validate("0.0e", None)
    assert result == QValidator.Invalid


def test_list_binding_float_validator():
    binding = VectorFloatBinding()
    validator = ListValidator(binding=binding)
    result, _, _ = validator.validate("", None)
    assert result == QValidator.Acceptable

    # A double regex can have integer input
    result, _, _ = validator.validate("1", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("0", None)
    assert result == QValidator.Acceptable

    # But not boolean
    result, _, _ = validator.validate("Fals", None)
    assert result == QValidator.Invalid

    # Floating input
    result, _, _ = validator.validate("0.", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("0.0", None)
    assert result == QValidator.Acceptable

    # Sign at the end
    result, _, _ = validator.validate("0-", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("0.0-", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("0.0+", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("0.0e", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("0.0e-7", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("+0000.1223", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("+m", None)
    assert result == QValidator.Invalid

    result, _, _ = validator.validate("+e", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("xfel", None)
    assert result == QValidator.Invalid


def test_list_validator_size():
    binding = VectorFloatBinding(attributes={"minSize": 1, "maxSize": 3})
    validator = ListValidator(binding=binding)

    # We are not allowed to be empty!
    result, _, _ = validator.validate("", None)
    assert result == QValidator.Intermediate

    # Accepted in limits
    result, _, _ = validator.validate("1", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("1,", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("1,2,3", None)
    assert result == QValidator.Acceptable

    # Out of limits
    result, _, _ = validator.validate("1,2,3,4", None)
    assert result == QValidator.Intermediate


def test_list_integer_limits():
    binding = VectorUint8Binding()
    validator = ListValidator(binding=binding)

    result, _, _ = validator.validate("1", None)
    assert result == QValidator.Acceptable

    result, _, _ = validator.validate("-1", None)
    assert result == QValidator.Invalid

    # Overflowing are intermediate for lists
    result, _, _ = validator.validate("2550", None)
    assert result == QValidator.Intermediate

    result, _, _ = validator.validate("255", None)
    assert result == QValidator.Acceptable


def test_simple_validator():
    binding = Uint8Binding()
    validator = SimpleValidator(binding=binding)
    result, _, _ = validator.validate("", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("1", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("-1", None)
    assert result == QValidator.Invalid
    result, _, _ = validator.validate("-", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("0", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("255", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("256", None)
    assert result == QValidator.Invalid
    result, _, _ = validator.validate("0001", None)
    assert result == QValidator.Intermediate

    # Test with binding attributes
    attributes = {KARABO_SCHEMA_MIN_INC: 2, KARABO_SCHEMA_MAX_EXC: 128}
    binding = Uint8Binding(attributes=attributes)
    validator = SimpleValidator(binding=binding)
    result, _, _ = validator.validate("", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("1", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("2", None)
    assert result == QValidator.Acceptable
    result, _, _ = validator.validate("-1", None)
    assert result == QValidator.Invalid
    result, _, _ = validator.validate("-", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("0", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("255", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("128", None)
    assert result == QValidator.Intermediate
    result, _, _ = validator.validate("127", None)
    assert result == QValidator.Acceptable
