#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from numbers import Number

from numpy import log10, ndarray, number
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QFrame, QLabel

from karabo.middlelayer import (Double, Float, String, Simple, Type, HashType,
                                VectorDouble, VectorFloat, VectorHash)
from karabo_gui.alarms.api import ALARM_COLOR, WARN_COLOR
from karabo_gui.const import FINE_COLOR, WIDGET_MIN_HEIGHT
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget
from .unitlabel import add_unit_label

# alpha layer to add to our global alarm and warn colors
ALPHA = (64,)


class DisplayLabel(DisplayWidget):
    category = String, Simple
    alias = "Value Field"
    priority = 20

    def __init__(self, box, parent):
        super(DisplayLabel, self).__init__(box)

        self.value = None

        self._internal_widget = QLabel(parent)
        self._internal_widget.setAlignment(Qt.AlignCenter)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.setWordWrap(True)
        self.widget = add_unit_label(box, self._internal_widget, parent=parent)
        self.widget.setFrameStyle(QFrame.Box | QFrame.Plain)

        objectName = generateObjectName(self)
        self._styleSheet = ("QWidget#{}".format(objectName) +
                            " {{ background-color : rgba{}; }}")
        self.widget.setObjectName(objectName)

        self.bg_color = FINE_COLOR

    def __checkAlarms(self, desc, value):
        if ((desc.alarmLow is not None and value < desc.alarmLow) or
                (desc.alarmHigh is not None and value > desc.alarmHigh)):
            self.bg_color = ALARM_COLOR + ALPHA
        elif ((desc.warnLow is not None and value < desc.warnLow) or
                (desc.warnHigh is not None and value > desc.warnHigh)):
            self.bg_color = WARN_COLOR + ALPHA
        else:
            self.bg_color = FINE_COLOR
        sheet = self._styleSheet.format(self.bg_color)
        self.widget.setStyleSheet(sheet)

    def valueChanged(self, box, value, timestamp=None):
        self.widget.updateLabel(box)

        desc = box.descriptor
        self.__checkAlarms(desc, value)

        if (not isinstance(desc, Type)
                or isinstance(desc, (HashType, VectorHash))):
            return  # only simple types can be shown here

        if value is None:
            return

        self.value = value

        if isinstance(value, str):
            # Make sure that long binary data (e.g. image) is not shown,
            # Otherwise slowness is the case
            self._internal_widget.setText(value[:255])
            return
        elif isinstance(value, (bytes, bytearray)):
            return

        try:
            format = dict(bin='b{:b}', oct='o{:o}', hex='0x{:X}'
                          )[desc.displayType[:3]]
        except (TypeError, KeyError):
            abserr = desc.absoluteError
            relerr = desc.relativeError
            if (relerr is not None and
                    (abserr is None or
                     not isinstance(value, (Number, number)) or
                     relerr * value > abserr)):
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

        self._internal_widget.setText(ret)
