from PyQt5.QtGui import QValidator

from karabogui.binding.api import (
    FloatBinding, VectorFloatBinding, VectorUint8Binding)

from ..validators import BindingValidator, ListValidator


def test_generic_binding_validator():
    binding = FloatBinding()
    validator = BindingValidator(binding=binding)
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
    binding = VectorFloatBinding(attributes={"minSize": 1, "maxSize":3})
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
