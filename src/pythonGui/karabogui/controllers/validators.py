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
import re
from ast import literal_eval

import numpy as np
from qtpy.QtGui import QValidator
from traits.api import TraitError

from karabogui.binding.api import (
    VectorBoolBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
    VectorInt64Binding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding,
    convert_string, get_min_max, get_min_max_size)

BOOL_REGEX = r"(0|1|[T]rue|[F]alse)"
INT_REGEX = r"^[-+]?\d+$"
UINT_REGEX = r"^[+]?\d+$"
STRING_REGEX = r"^.+$"
# Was before r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$"
DOUBLE_REGEX = r"^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$"

_CAST_MAP = {
    VectorInt8Binding: np.int8,
    VectorInt16Binding: np.int16,
    VectorInt32Binding: np.int32,
    VectorInt64Binding: np.int64,
    VectorUint8Binding: np.uint8,
    VectorUint16Binding: np.uint16,
    VectorUint32Binding: np.uint32,
    VectorUint64Binding: np.uint64,
}

_REGEX = {
    VectorBoolBinding: BOOL_REGEX,
    VectorDoubleBinding: DOUBLE_REGEX,
    VectorFloatBinding: DOUBLE_REGEX,
    VectorInt8Binding: INT_REGEX,
    VectorInt16Binding: INT_REGEX,
    VectorInt32Binding: INT_REGEX,
    VectorInt64Binding: INT_REGEX,
    VectorStringBinding: STRING_REGEX,
    VectorUint8Binding: UINT_REGEX,
    VectorUint16Binding: UINT_REGEX,
    VectorUint32Binding: UINT_REGEX,
    VectorUint64Binding: UINT_REGEX,
}

_INTERMEDIATE = {
    VectorBoolBinding: ("T", "t", "r", "u", "F", "a", "l", "s", ","),
    VectorDoubleBinding: ("+", "-", ",", "e"),
    VectorFloatBinding: ("+", "-", ",", ".", "e"),
    VectorInt8Binding: ("+", "-", ","),
    VectorInt16Binding: ("+", "-", ","),
    VectorInt32Binding: ("+", "-", ","),
    VectorInt64Binding: ("+", "-", ","),
    VectorStringBinding: (",",),
    VectorUint8Binding: ("+", ","),
    VectorUint16Binding: ("+", ","),
    VectorUint32Binding: ("+", ","),
    VectorUint64Binding: ("+", ","),
}


class ListValidator(QValidator):
    pattern = None
    intermediate = ()

    def __init__(self, binding, parent=None):
        super().__init__(parent=parent)
        self._binding = binding
        binding_type = type(self._binding)
        self.pattern = re.compile(_REGEX.get(binding_type, ""))
        self.intermediate = _INTERMEDIATE.get(binding_type, ())
        self.literal = (str if binding_type == VectorStringBinding
                        else literal_eval)
        # Note: Vector of integers are downcasted. We get a function
        # to validate that the user input matches the casted value
        self.cast = _CAST_MAP.get(binding_type, self.literal)

        minSize, maxSize = get_min_max_size(binding)
        self.minSize = minSize
        self.maxSize = maxSize

    def validate(self, input, pos):
        """The main validate function of the ListValidator"""
        # 1. Basic empty input check. Empty input might be acceptable!
        if input == "" and (self.minSize is None or not self.minSize):
            return self.Acceptable, input, pos
        elif input == "" and self.minSize is not None and self.minSize > 0:
            return self.Intermediate, input, pos
        elif input.startswith(","):
            return self.Intermediate, input, pos

        # 2. Check for size of list first
        values = [val for val in input.split(",")]
        if self.minSize is not None and len(values) < self.minSize:
            return self.Intermediate, input, pos
        if self.maxSize is not None and len(values) > self.maxSize:
            return self.Intermediate, input, pos

        # 3. Intermediate positions
        if input[-1] in self.intermediate:
            return self.Intermediate, input, pos

        # 4. Check every single list value if it is valid
        for value in values:
            if not self.pattern.match(value):
                return self.Invalid, input, pos
            # More checks including:
            # 4.1 We have to see we can cast it properly (SyntaxError)
            # 4.2 Check for changes in between Vectors (ValueError)
            # 4.3 Casts down integers (AssertionError), limits
            # 4.4 Overflowing integers (OverflowError)
            try:
                assert self.literal(value) == self.cast(value)
            except (ValueError, SyntaxError, AssertionError, OverflowError):
                return self.Intermediate, input, pos

        return self.Acceptable, input, pos


class BindingValidator(QValidator):
    """This is a soft generic binding validator

    Returns `Intermediate` if the binding cannot be validated by the trait. If
    the value can be validated by the binding trait it returns `Acceptable`.
    """

    def __init__(self, binding, parent=None):
        super().__init__(parent)
        self._binding = binding

    def validate(self, input, pos):
        """The main validate function using binding validate"""
        try:
            self._binding.validate_trait("value", input)
        except TraitError:
            return self.Intermediate, input, pos

        return self.Acceptable, input, pos


class SimpleValidator(QValidator):
    """This is a numeric binding validator which accounts min and max limits
    """

    def __init__(self, binding, parent=None):
        super().__init__(parent=parent)
        self._binding = binding
        self.minInc, self.maxInc = get_min_max(binding)

    def validate(self, input, pos):
        """Reimplemented function of QValidator to validate numeric input"""
        if input in ("+", "-", ""):
            return self.Intermediate, input, pos
        elif input[-1] in (" "):
            return self.Invalid, input, pos
        elif input[-1] in ("+", "-", "e"):
            return self.Intermediate, input, pos

        # Use the fast path validation
        try:
            self._binding.validate_trait("value", input)
        except TraitError:
            return self.Invalid, input, pos

        if self.inside_limits(input):
            return self.Acceptable, input, pos

        return self.Intermediate, input, pos

    def inside_limits(self, value):
        """Check if a value is within limits"""
        value, success = convert_string(value)
        if not success:
            return False
        if value < self.minInc or value > self.maxInc:
            return False

        # Check passed!
        return True
