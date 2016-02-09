#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayLabel"]


from karabo_gui.widget import DisplayWidget
from karabo.api_2 import (Double, Float, Hash, String, Simple, Type, HashType,
                          VectorDouble, VectorFloat, VectorHash)

from enum import Enum
from numbers import Number
import decimal
import re

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLabel

from numpy import log10, ndarray, number


class ErrorState(Enum):
    """this is the state as shown by the background color"""
    fine = "225,242,225,128"
    warn = "255,255,125,128"
    alarm = "255,125,125,128"
    error = "225,155,155,128"


class DisplayLabel(DisplayWidget):
    category = String, Simple
    alias = "Value Field"
  
    def __init__(self, box, parent):
        super(DisplayLabel, self).__init__(box)

        self.value = None

        self.widget = QLabel(parent)
        self.widget.setAutoFillBackground(True)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setMinimumWidth(160)
        self.widget.setMinimumHeight(32)
        self.widget.setWordWrap(True)
        self.inError = False
        self.errorState = ErrorState.fine

    def setErrorState(self, inError):
        self.inError = inError
        self.setBackground()

    def setBackground(self):
        """set the background color

        this is determined from the error, warn and alarm state"""
        if self.inError:
            state = ErrorState.error
        else:
            state = self.errorState

        self.widget.setStyleSheet("QLabel {{ background-color : rgba({}); }}".
                                  format(state.value))

    def __checkAlarms(self, desc, value):
        if ((desc.alarmLow is not None and value < desc.alarmLow) or
                (desc.alarmHigh is not None and value > desc.alarmHigh)):
            self.errorState = ErrorState.alarm
        elif ((desc.warnLow is not None and value < desc.warnLow) or
                (desc.warnHigh is not None and value > desc.warnHigh)):
            self.errorState = ErrorState.warn
        else:
            self.errorState = ErrorState.fine
        self.setBackground()

    def valueChanged(self, box, value, timestamp=None):
        desc = box.descriptor

        self.__checkAlarms(desc, value)

        if (not isinstance(desc, Type)
            or isinstance(desc, (HashType, VectorHash))):
            return # only simple types can be shown here

        if value is None:
            return

        self.value = value

        if isinstance(value, str):
            self.widget.setText(value[:30])
            return
        elif isinstance(value, bytes):
            return

        try:
            format = dict(bin='b{:b}', oct='o{:o}', hex='0x{:X}'
                          )[desc.displayType[:3]]
        except (TypeError, KeyError):
            abserr = desc.absoluteError
            relerr = desc.relativeError
            if relerr is not None and (abserr is None or
                     not isinstance(value, (Number, number)) or
                     relerr * value > abserr):
                format = "{{:.{}g}}".format(
                            -int(log10(desc.relativeError)))
            elif abserr is not None:
                if abserr < 1:
                    format = "{{:.{}f}}".format(-int(log10(abserr)))
                elif (isinstance(value, (Number, number)) and
                        abs(value) > abserr):
                    format = "{{:.{}e}}".format(int(log10(abs(value))) -
                                                int(log10(abserr)))
                else:
                    format = "{:.0f}"
            elif isinstance(desc, (Float, VectorFloat)):
                format = "{:.6g}"
            elif isinstance(desc, (Double, VectorDouble)):
                format = "{:.10g}"
            else:
                format = "{}"

        if isinstance(value, (list, ndarray)):
            ret = '[' + ', '.join(format.format(v) for v in value[:10])
            if len(value) > 10:
                ret += ', ..]'
            else:
                ret += ']'
        else:
            ret = format.format(value)

        self.widget.setText(ret)
