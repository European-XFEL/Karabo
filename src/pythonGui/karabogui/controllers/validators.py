#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 25, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtGui import QValidator


class HexValidator(QValidator):
    def __init__(self, min=None, max=None, parent=None):
        QValidator.__init__(self, parent)
        self.min = min
        self.max = max

    def is_hex(self, input):
        return all(i in "0123456789abcdefABCDEF" for i in input)

    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (self.is_hex(input) or
                input[0] in '+-' and self.is_hex(input[1:])):
            return self.Invalid, input, pos

        if self.min is not None and self.min >= 0 and input.startswith('-'):
            return self.Invalid, input, pos

        if self.max is not None and self.max < 0 and input.startswith('+'):
            return self.Invalid, input, pos

        if ((self.min is None or self.min <= int(input, base=16)) and
                (self.max is None or int(input, base=16) <= self.max)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, min):
        self.min = min

    def setTop(self, max):
        self.max = max


class IntValidator(QValidator):
    def __init__(self, min=None, max=None, parent=None):
        QValidator.__init__(self, parent)
        self.min = min
        self.max = max

    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        if self.min is not None and self.min >= 0 and input.startswith('-'):
            return self.Invalid, input, pos

        if self.max is not None and self.max < 0 and input.startswith('+'):
            return self.Invalid, input, pos

        if ((self.min is None or self.min <= int(input)) and
                (self.max is None or int(input) <= self.max)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, min):
        self.min = min

    def setTop(self, max):
        self.max = max


class NumberValidator(QValidator):
    def __init__(self, min=None, max=None, decimals=-1, parent=None):
        QValidator.__init__(self, parent)
        self.min = min
        self.max = max
        self.decimals = decimals

    def validate(self, input, pos):
        # start input check
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        # we might not have standard notation
        if input[-1] in ('+', '-', 'e'):
            return self.Intermediate, input, pos
        # we might not have standard notation
        elif input[-1] in (' '):
            return self.Invalid, input, pos

        # check for floating point precision
        if self.decimals != -1:
            input_split = input.split('.')
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
        self.min = min

    def setTop(self, max):
        self.max = max
