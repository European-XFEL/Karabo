#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 25, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import re

from qtpy.QtGui import QValidator


class HexValidator(QValidator):
    def __init__(self, min=None, max=None, parent=None):
        super().__init__(parent)
        self.min = min
        self.max = max

    def is_hex(self, input):
        return all(i in "0123456789abcdefABCDEF" for i in input)

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if input in ("+", "-", ""):
            return self.Intermediate, input, pos

        if not (self.is_hex(input) or
                input[0] in "+-" and self.is_hex(input[1:])):
            return self.Invalid, input, pos

        if self.min is not None and self.min >= 0 and input.startswith("-"):
            return self.Invalid, input, pos

        if self.max is not None and self.max < 0 and input.startswith("+"):
            return self.Invalid, input, pos

        if ((self.min is None or self.min <= int(input, base=16)) and
                (self.max is None or int(input, base=16) <= self.max)):
            return self.Acceptable, input, pos

        return self.Intermediate, input, pos

    def setBottom(self, min):
        self.min = min

    def setTop(self, max):
        self.max = max


class IntValidator(QValidator):
    def __init__(self, min=None, max=None, parent=None):
        super().__init__(parent)
        self.min = min
        self.max = max

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if input in ("+", "-", ""):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in "+-" and input[1:].isdigit()):
            return self.Invalid, input, pos

        if self.min is not None and self.min >= 0 and input.startswith("-"):
            return self.Invalid, input, pos

        if self.max is not None and self.max < 0 and input.startswith("+"):
            return self.Invalid, input, pos

        if ((self.min is None or self.min <= int(input)) and
                (self.max is None or int(input) <= self.max)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, min):
        """Reimplemented function of `QValidator`"""
        self.min = min

    def setTop(self, max):
        """Reimplemented function of `QValidator`"""
        self.max = max


class NumberValidator(QValidator):
    def __init__(self, min=None, max=None, decimals=-1, parent=None):
        super().__init__(parent)
        self.min = min
        self.max = max
        self.decimals = decimals

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if input in ("+", "-", ""):
            return self.Intermediate, input, pos

        # we might not have standard notation
        if input[-1] in ("+", "-", "e"):
            return self.Intermediate, input, pos
        # we might not have standard notation
        elif input[-1] in (" "):
            return self.Invalid, input, pos

        # check for floating point precision
        if self.decimals != -1:
            input_split = input.split(".")
            if len(input_split) > 1 and len(input_split[1]) > self.decimals:
                return self.Invalid, input, pos

        # try if we can cast input
        try:
            value = float(input)
        except ValueError:
            return self.Invalid, input, pos
        # then check for limits
        if ((self.min is None or self.min <= value) and
                (self.max is None or value <= self.max)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, min):
        """Reimplemented function of `QValidator`"""
        self.min = min

    def setTop(self, max):
        """Reimplemented function of `QValidator`"""
        self.max = max


class RegexValidator(QValidator):
    """This is a soft generic regex validator that accepts a regex pattern

    Returns `Intermediate` if the pattern does not match, otherwise
    `Acceptable`!
    """
    pattern = None

    def __init__(self, pattern="", parent=None):
        super().__init__(parent)
        self.pattern = re.compile(pattern)

    def setRegex(self, pattern):
        """Set a new regex pattern"""
        self.pattern = re.compile(pattern)

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if not self.pattern.match(input):
            return self.Intermediate, input, pos

        return self.Acceptable, input, pos


class RegexListValidator(QValidator):
    """This is a list generic regex validator that accepts a regex pattern

    Every element is validated against the regex pattern

    :param regex: the regex expression
    :param delimiter: the delimiter (default: ".") which splits the input
    :param min_size: The minimum size of the list (default: None)
    :param max_size: The maximum size of the list (default: None)
    """
    pattern = None

    def __init__(self, pattern="", delimiter=",", min_size=None, max_size=None,
                 parent=None):
        super().__init__(parent)
        self.pattern = re.compile(pattern)
        self.delimiter = delimiter
        self.min_size = min_size
        self.max_size = max_size

    def setRegex(self, pattern):
        """Set a new regex pattern"""
        self.pattern = re.compile(pattern)

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if not input and (self.min_size is None or self.min_size == 0):
            return self.Acceptable, input, pos
        elif not input and self.min_size is not None and self.min_size > 0:
            return self.Intermediate, input, pos
        elif input.startswith(self.delimiter):
            return self.Intermediate, input, pos

        # check for size first
        values = input.split(self.delimiter)
        if self.min_size is not None and len(values) < self.min_size:
            return self.Intermediate, input, pos
        if self.max_size is not None and len(values) > self.max_size:
            return self.Intermediate, input, pos

        if input.endswith(self.delimiter):
            return self.Intermediate, input, pos

        for string in values:
            if not self.pattern.match(string):
                return self.Intermediate, input, pos

        return self.Acceptable, input, pos
