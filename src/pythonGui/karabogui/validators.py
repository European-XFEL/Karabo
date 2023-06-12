#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 25, 2019
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
import re

from qtpy.QtGui import QValidator


class HexValidator(QValidator):
    def __init__(self, minInc=None, maxInc=None, parent=None):
        super().__init__(parent)
        self.minInc = minInc
        self.maxInc = maxInc

    def is_hex(self, input):
        return all(i in "0123456789abcdefABCDEF" for i in input)

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if input in ("+", "-", ""):
            return self.Intermediate, input, pos

        if not (self.is_hex(input) or
                input[0] in "+-" and self.is_hex(input[1:])):
            return self.Invalid, input, pos

        minInc = self.minInc
        if minInc is not None and minInc >= 0 and input.startswith("-"):
            return self.Invalid, input, pos

        maxInc = self.maxInc
        if maxInc is not None and maxInc < 0 and input.startswith("+"):
            return self.Invalid, input, pos

        if ((minInc is None or minInc <= int(input, base=16)) and
                (maxInc is None or int(input, base=16) <= maxInc)):
            return self.Acceptable, input, pos

        return self.Intermediate, input, pos

    def setBottom(self, minInc):
        self.minInc = minInc

    def setTop(self, maxInc):
        self.maxInc = maxInc


class IntValidator(QValidator):
    def __init__(self, minInc=None, maxInc=None, parent=None):
        super().__init__(parent)
        self.minInc = minInc
        self.maxInc = maxInc

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if input in ("+", "-", ""):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in "+-" and input[1:].isdigit()):
            return self.Invalid, input, pos

        minInc = self.minInc
        if minInc is not None and minInc >= 0 and input.startswith("-"):
            return self.Invalid, input, pos

        maxInc = self.maxInc
        if maxInc is not None and maxInc < 0 and input.startswith("+"):
            return self.Invalid, input, pos

        if ((minInc is None or minInc <= int(input)) and
                (maxInc is None or int(input) <= maxInc)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, minInc):
        """Reimplemented function of `QValidator`"""
        self.minInc = minInc

    def setTop(self, maxInc):
        """Reimplemented function of `QValidator`"""
        self.maxInc = maxInc


class NumberValidator(QValidator):
    def __init__(self, minInc=None, maxInc=None, decimals=-1, parent=None):
        super().__init__(parent)
        self.minInc = minInc
        self.maxInc = maxInc
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
        if ((self.minInc is None or self.minInc <= value) and
                (self.maxInc is None or value <= self.maxInc)):
            return self.Acceptable, input, pos

        return self.Intermediate, input, pos

    def setBottom(self, minInc):
        """Reimplemented function of `QValidator`"""
        self.minInc = minInc

    def setTop(self, maxInc):
        """Reimplemented function of `QValidator`"""
        self.maxInc = maxInc


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
    """This is a generic list regex validator that accepts a regex pattern

    Every element is validated against the regex pattern

    :param regex: the regex expression
    :param delimiter: the delimiter (default: ".") which splits the input
    :param minSize: The min size of the list (default: None)
    :param maxSize: The max size of the list (default: None)
    """

    def __init__(self, pattern="", delimiter=",", minSize=None, maxSize=None,
                 parent=None):
        super().__init__(parent)
        self.pattern = re.compile(pattern)
        self.delimiter = delimiter
        self.minSize = minSize
        self.maxSize = maxSize

    def setRegex(self, pattern):
        """Set a new regex pattern"""
        self.pattern = re.compile(pattern)

    def validate(self, input, pos):
        """Reimplemented function of `QValidator`"""
        if not input and (self.minSize is None or self.minSize == 0):
            return self.Acceptable, input, pos
        elif not input and self.minSize is not None and self.minSize > 0:
            return self.Intermediate, input, pos
        elif input.startswith(self.delimiter):
            return self.Intermediate, input, pos

        # check for size first
        values = input.split(self.delimiter)
        if self.minSize is not None and len(values) < self.minSize:
            return self.Intermediate, input, pos
        if self.maxSize is not None and len(values) > self.maxSize:
            return self.Intermediate, input, pos

        if input.endswith(self.delimiter):
            return self.Intermediate, input, pos

        for string in values:
            if not self.pattern.match(string):
                return self.Intermediate, input, pos

        return self.Acceptable, input, pos
