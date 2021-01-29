from PyQt5.QtGui import QValidator

from karabogui.binding.api import (
    FloatBinding, VectorFloatBinding)

from ..validators import BindingValidator


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
