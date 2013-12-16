#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 10, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which is derived from QDoubleSpinBox and is able
   to show its values in scientific notation.
"""

__all__ = ["ScientificDoubleSpinBox"]

import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ScientificDoubleSpinBox(QDoubleSpinBox):

    def __init__(self, parent=None):
        super(ScientificDoubleSpinBox, self).__init__(parent)
        
        self.displayDecimals = 0
        self.delimiter = QChar()
        
        self.cachedText = ""
        self.cachedState = QValidator.State
        self.cachedValue = None
        
        self.setDecimals(4)
        QDoubleSpinBox.setDecimals(self, 1000)
        
	# Set Range to maximum possible values
	doubleMax = sys.float_info.max
	self.setRange(-doubleMax, doubleMax)


    def decimals(self):
        return self.displayDecimals


    def setDecimals(self, value):
        self.displayDecimals = value


    def stepBy(self, steps):
        if steps < 0:
            self.stepDown()
        else:
            self.stepUp()


    def stepDown(self):
        self.setValue(self.value()/10.0)


    def stepUp(self):
        self.setValue(self.value()*10.0)


    # Overwritten virtual function from QDoubleSpinBox
    def validate(self, input, pos):
        state = QValidator.State
        state, value = self.validateAndInterpret(input, pos, state)
        
        return (state, pos)


    # Overwritten virtual function from QDoubleSpinBox
    def textFromValue(self, value):
        str = self.locale().toString(value, 'e', self.decimals())
        if abs(value) >= 1000.0:
            str.remove(self.locale().groupSeparator())
        
        return str
    

    # Overwritten virtual function from QDoubleSpinBox
    def valueFromText(self, text):
        state = QValidator.State
        pos = self.lineEdit().cursorPosition()
        state, value = self.validateAndInterpret(text, pos, state)
        
        return float(value)


    # Function from QDoubleSpinBoxPrivate converted into Python
    def validateAndInterpret(self, input, pos, state):
        if (self.cachedText == input) and (not input.isEmpty()):
            state = self.cachedState
            return (state, self.cachedValue)
        
        max = sys.float_info.max
        min = -max

        copy, pos = self.stripped(input, pos)

        len = copy.size()
        num = min
        plus = max >= 0
        minus = min <= 0

        if len == 0:
            if max != min:
                state = QValidator.Intermediate
            else:
                state = QValidator.Invalid
            return self.checkValidated(min, max, state, copy, num)
        elif len == 1:
            if (copy.at(0) == self.delimiter
                or (plus and copy.at(0) == QLatin1Char('+'))
                or (minus and copy.at(0) == QLatin1Char('-'))):
                state = QValidator.Intermediate;
                return self.checkValidated(min, max, state, copy, num)
        elif len == 2:
            if (copy.at(1) == self.delimiter
                and ((plus and copy.at(0) == QLatin1Char('+')) or (minus and copy.at(0) == QLatin1Char('-')))):
                state = QValidator.Intermediate
                return self.checkValidated(min, max, state, copy, num)

        if copy.at(0) == self.locale().groupSeparator():
            print "state is set to Invalid"
            state = QValidator.Invalid
            return self.checkValidated(min, max, state, copy, num)
        elif len > 1:
            dec = copy.indexOf(self.delimiter)
            if dec != -1:
                if (dec+1 < copy.size() and copy.at(dec+1) == self.delimiter and pos == dec+1):
                    copy.remove(dec + 1, 1) # typing a delimiter when you are on the delimiter
                # should be treated as typing right arrow
                if copy.size()-dec > self.decimals()+1:
                    print "state is set to Invalid"
                    state = QValidator.Invalid
                    return self.checkValidated(min, max, state, copy, num)

                for i in range(dec+1, copy.size()):
                    if copy.at(i).isSpace() or copy.at(i) == self.locale().groupSeparator():
                        print "state is set to Invalid"
                        state = QValidator.Invalid
                        return self.checkValidated(min, max, state, copy, num)
 
            else:
                last = copy.at(len-1)
                secondLast = copy.at(len-2)
                if ((last == self.locale().groupSeparator() or last.isSpace())
                    and (secondLast == self.locale().groupSeparator() or secondLast.isSpace())):
                    state = QValidator.Invalid
                    print "state is set to Invalid"
                    return self.checkValidated(min, max, state, copy, num)
                elif (last.isSpace() and (not self.locale().groupSeparator().isSpace() or secondLast.isSpace())):
                    state = QValidator.Invalid
                    print "state is set to Invalid"
                    return self.checkValidated(min, max, state, copy, num)

        ok = True
        try:
            num = float(self.locale())
        except ValueError:
            ok = False

        if not ok:
            if self.locale().groupSeparator().isPrint():
                if (max < 1000 and min > -1000 and copy.contains(self.locale().groupSeparator())):
                    state = QValidator.Invalid
                    print "state is set to Invalid"
                    return self.checkValidated(min, max, state, copy, num)
                
                len = copy.size()
                for i in range(0, len-1):
                    if (copy.at(i) == self.locale().groupSeparator() and copy.at(i + 1) == self.locale.groupSeparator()):
                        print "state is set to Invalid"
                        state = QValidator.Invalid
                        return self.checkValidated(min, max, state, copy, num)

                copy2 = copy
                copy2.remove(self.locale().groupSeparator())
                try:
                    num = float(self.locale())
                    ok = True
                except ValueError:
                    state = QValidator.Invalid
                    return self.checkValidated(min, max, state, copy, num)

        if not ok:
            state = QValidator.Invalid
            print "state is set to Invalid"
        elif (num >= min and num <= max):
            state = QValidator.Acceptable
        elif (max == min): # when max and min is the same the only non-Invalid input is max (or min)
            state = QValidator.Invalid
            print "state is set to Invalid"
        else:
            if ((num >= 0 and num > max) or (num < 0 and num < min)):
                state = QValidator.Invalid
                print "state is set to Invalid"
            else:
                state = QValidator.Intermediate
                print "state is set to Intermediate"

        return self.checkValidated(min, max, state, copy, num)


    def stripped(self, text, pos):
        specialValueText = self.specialValueText()
        
        if specialValueText.size() == 0 or text != specialValueText:
            startPos = 0
            size = text.size()
            changed = False
            prefix = self.prefix()
            suffix = self.suffix()
            
            if prefix.size() and text.startsWith(prefix):
                startPos += prefix.size()
                size -= startPos
                changed = True

            if suffix.size() and text.endsWith(suffix):
                size -= suffix.size()
                changed = True

            if changed:
                text = text.mid(startPos, size)

        s = text.size()
        text = text.trimmed()
        if pos:
            pos -= s - text.size()
        return text, pos


    def checkValidated(self, min, max, state, copy, num):
        if state != QValidator.Acceptable:
            if max > 0:
                num = min
            else:
                num = max

        input = self.prefix() + copy + self.suffix()
        self.cachedText = input
        self.cachedState = state
        self.cachedValue = num
        
        return state, num

