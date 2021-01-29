from ast import literal_eval
import re

import numpy as np
from PyQt5.QtGui import QValidator

from karabogui.binding.api import (
    validate_value, VectorBoolBinding, VectorComplexDoubleBinding,
    VectorComplexFloatBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
    VectorInt64Binding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding)

INT_REGEX = r"^[-+]?\d+$"
UINT_REGEX = r"^[+]?\d+$"
DOUBLE_REGEX = r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$"

CAST_MAP = {
    VectorInt8Binding: np.int8,
    VectorInt16Binding: np.int16,
    VectorInt32Binding: np.int32,
    VectorInt64Binding: np.int64,
    VectorUint8Binding: np.uint8,
    VectorUint16Binding: np.uint16,
    VectorUint32Binding: np.uint32,
    VectorUint64Binding: np.uint64,
}

REGEX_MAP = {
    VectorBoolBinding: r"(0|1|[T]rue|[F]alse)",
    VectorComplexDoubleBinding: DOUBLE_REGEX,  # XXX
    VectorComplexFloatBinding: DOUBLE_REGEX,  # XXX
    VectorDoubleBinding: DOUBLE_REGEX,
    VectorFloatBinding: DOUBLE_REGEX,
    VectorInt8Binding: INT_REGEX,
    VectorInt16Binding: INT_REGEX,
    VectorInt32Binding: INT_REGEX,
    VectorInt64Binding: INT_REGEX,
    VectorStringBinding: r"^.+$",
    VectorUint8Binding: UINT_REGEX,
    VectorUint16Binding: UINT_REGEX,
    VectorUint32Binding: UINT_REGEX,
    VectorUint64Binding: UINT_REGEX,
}

MEDIATE_MAP = {
    VectorBoolBinding: ('T', 't', 'r', 'u', 'F', 'a', 'l', 's', ','),
    VectorComplexDoubleBinding: ('+', '-', 'j', ','),  # X
    VectorComplexFloatBinding: ('+', '-', 'j', ','),  # X
    VectorDoubleBinding: ('+', '-', ','),
    VectorFloatBinding: ('+', '-', ',', '.'),
    VectorInt8Binding: ('+', '-', ','),
    VectorInt16Binding: ('+', '-', ','),
    VectorInt32Binding: ('+', '-', ','),
    VectorInt64Binding: ('+', '-', ','),
    VectorStringBinding: (','),
    VectorUint8Binding: ('+', ','),
    VectorUint16Binding: ('+', ','),
    VectorUint32Binding: ('+', ','),
    VectorUint64Binding: ('+', ','),
}


class ListValidator(QValidator):
    pattern = None
    intermediate = ()

    def __init__(self, binding=None, min_size=None, max_size=None,
                 parent=None):
        super(ListValidator, self).__init__(parent)
        bind_type = type(binding)
        self.pattern = re.compile(REGEX_MAP.get(bind_type, ''))
        self.intermediate = MEDIATE_MAP.get(bind_type, ())
        self.literal = (str if bind_type == VectorStringBinding
                        else literal_eval)
        # vector of integers are downcasted, here we get a function
        # to validate that the user input matches the casted value
        self.cast = CAST_MAP.get(bind_type, self.literal)
        self.min_size = min_size
        self.max_size = max_size

    def validate(self, input, pos):
        """The main validate function
        """
        # completely empty input might be acceptable!
        if input in ('', []) and (self.min_size is None or self.min_size == 0):
            return self.Acceptable, input, pos
        elif (input in ('', []) and self.min_size is not None
              and self.min_size > 0):
            return self.Intermediate, input, pos
        elif input.startswith(','):
            return self.Intermediate, input, pos

        # check for size first
        values = [val for val in input.split(',')]
        if self.min_size is not None and len(values) < self.min_size:
            return self.Intermediate, input, pos
        if self.max_size is not None and len(values) > self.max_size:
            return self.Intermediate, input, pos

        # intermediate positions
        if input[-1] in self.intermediate:
            return self.Intermediate, input, pos

        # check every single list value if it is valid
        for value in values:
            if not self.pattern.match(value):
                return self.Invalid, input, pos
            # We have to check other behavior, e.g. leading zeros, and see
            # if we can cast it properly (SyntaxError)
            # Check for changes in between Vectors (ValueError)
            # or if self.cast casts down integers (AssertionError)
            try:
                assert self.literal(value) == self.cast(value)
            except (ValueError, SyntaxError, AssertionError):
                return self.Intermediate, input, pos

        return self.Acceptable, input, pos


class BindingValidator(QValidator):
    """This is a soft generic binding validator

    Returns `Intermediate` if the binding cannot be validated by the trait. If
    the value can be validated by the binding trait it returns `Acceptable`.
    """

    def __init__(self, binding, parent=None):
        super(BindingValidator, self).__init__(parent)
        self.binding = binding

    def validate(self, input, pos):
        """The main validate function using binding validate"""
        valid = validate_value(self.binding, input)
        if valid is None:
            return self.Intermediate, input, pos

        return self.Acceptable, input, pos
