#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from enum import Enum
from numbers import Number

from numpy import log10, ndarray, number
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLabel

from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import (Double, Float, String, Simple, Type, HashType,
                                VectorDouble, VectorFloat, VectorHash)


class ErrorState(Enum):
    """this is the state as shown by the background color"""
    fine = OK_COLOR
    warn = (255, 255, 125, 128)
    alarm = (255, 125, 125, 128)
    error = ERROR_COLOR_ALPHA


class DisplayLabel(DisplayWidget):
    category = String, Simple
    alias = "Value Field"
  
    def __init__(self, box, parent):
        super(DisplayLabel, self).__init__(box)

        self.value = None

        self.widget = QLabel(parent)
        self.widget.setAutoFillBackground(True)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setMinimumHeight(21)
        self.widget.setWordWrap(True)

        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; }}")
        self.widget.setObjectName(objectName)
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

        ss = self._styleSheet.format(state.value)
        self.widget.setStyleSheet(ss)

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
            # Make sure that long binary data (e.g. image) is not shown,
            # Otherwise slowness is the case
            self.widget.setText(value[:255])
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
