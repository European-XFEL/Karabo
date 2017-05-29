#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 5, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from PyQt4.QtCore import Qt, QRectF
from PyQt4.QtGui import (QColor, QLabel, QMessageBox, QPainter, QPainterPath,
                         QPixmap, QPen)

from karabo.middlelayer import Float
from karabo_gui.alarms.api import ALARM_COLOR, NORM_COLOR, WARN_COLOR
from karabo_gui.widget import DisplayWidget

B_ALOW, B_WLOW, B_WHIGH, B_AHIGH = (0.1, 0.3, 0.7, 0.9)
DARKER_GREY = (64, 64, 64)
DARK_GREY = (125, 135, 135)
GREY = (200, 200, 200)
MARGIN = 5


class DisplayAnalog(DisplayWidget):
    category = Float
    alias = "Analog Widget"

    _height = 180
    _width = 26

    def __init__(self, box, parent):
        super(DisplayAnalog, self).__init__(box)

        self.widget = QLabel(parent)

        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setFixedSize(self._width, self._height)
        self.value = None

    def typeChanged(self, box):
        desc = box.descriptor
        warning = {"LOW": desc.warnLow,
                   "HIGH": desc.warnHigh}
        alarm = {"LOW": desc.alarmLow,
                 "HIGH": desc.alarmHigh}
        if None in alarm.values() and None in warning.values():
            QMessageBox.warning(None, "Wrong property configuration",
                                "No proper configuration detected.\n"
                                "Please define alarm and warning thresholds.")

    def drawRegion(self, painter, start, stop, color):
        rect = QRectF(MARGIN, start * self._height,
                      self._width - 2 * MARGIN,
                      (stop - start) * self._height)
        painter.fillRect(rect, QColor(*color))

    def drawEmpty(self, painter):
        self.drawRegion(painter, B_AHIGH, 1, DARKER_GREY)
        self.drawRegion(painter, B_WHIGH, B_AHIGH, DARK_GREY)
        self.drawRegion(painter, B_WLOW, B_WHIGH, GREY)
        self.drawRegion(painter, B_ALOW, B_WLOW, DARK_GREY)
        self.drawRegion(painter, 0, B_ALOW, DARKER_GREY)
        w_value = 0.5

        return w_value

    def drawOnlyAlarms(self, painter, alarm, value):
        self.drawRegion(painter, B_AHIGH, 1, ALARM_COLOR)
        self.drawRegion(painter, B_ALOW, B_AHIGH, NORM_COLOR)
        self.drawRegion(painter, 0, B_ALOW, ALARM_COLOR)

        if value >= alarm["LOW"] and value <= alarm["HIGH"]:
            max_min = alarm["HIGH"] - alarm["LOW"]
            norm_region = (value - alarm["LOW"]) / max_min
            w_value = norm_region * (B_AHIGH - B_ALOW) + B_ALOW
        elif value > alarm["HIGH"]:
            w_value = (1 - B_AHIGH) / 2 + B_AHIGH
        elif value < alarm["LOW"]:
            w_value = B_ALOW / 2

        return w_value

    def drawOnlyWarnings(self, painter, warning, value):
        self.drawRegion(painter, B_WHIGH, 1, WARN_COLOR)
        self.drawRegion(painter, B_WLOW, B_WHIGH, NORM_COLOR)
        self.drawRegion(painter, 0, B_WLOW, WARN_COLOR)

        if value >= warning["LOW"] and value <= warning["HIGH"]:
            max_min = warning["HIGH"] - warning["LOW"]
            norm_region = (value - warning["LOW"]) / max_min
            w_value = norm_region * (B_WHIGH - B_WLOW) + B_WLOW
        elif value > warning["HIGH"]:
            w_value = (1 - B_WHIGH) / 2 + B_WHIGH
        elif value < warning["LOW"]:
            w_value = B_WLOW / 2

        return w_value

    def drawFull(self, painter, warning, alarm, value):
        self.drawRegion(painter, B_AHIGH, 1, ALARM_COLOR)
        self.drawRegion(painter, B_WHIGH, B_AHIGH, WARN_COLOR)
        self.drawRegion(painter, B_WLOW, B_WHIGH, NORM_COLOR)
        self.drawRegion(painter, B_ALOW, B_WLOW, WARN_COLOR)
        self.drawRegion(painter, 0, B_ALOW, ALARM_COLOR)

        if value >= warning["LOW"] and value <= warning["HIGH"]:
            max_min = warning["HIGH"] - warning["LOW"]
            norm_region = (value - warning["LOW"]) / max_min
            w_value = norm_region * (B_WHIGH - B_WLOW) + B_WLOW
        elif value >= alarm["HIGH"]:
            w_value = (1 - B_AHIGH) / 2 + B_AHIGH
        elif value <= alarm["LOW"]:
            w_value = B_ALOW / 2
        elif value > alarm["LOW"] and value < warning["LOW"]:
            max_min = warning["LOW"] - alarm["LOW"]
            norm_region = (value - alarm["LOW"]) / max_min
            w_value = norm_region * (B_WLOW - B_ALOW) + B_ALOW
        elif value < alarm["HIGH"] and value > warning["HIGH"]:
            max_min = alarm["HIGH"] - warning["HIGH"]
            norm_region = (value - warning["HIGH"]) / max_min
            w_value = norm_region * (B_AHIGH - B_WHIGH) + B_WHIGH

        return w_value

    def paintWidget(self, value, warning, alarm):
        pixmap = QPixmap(self._width, self._height)
        pixmap.fill(QColor(Qt.transparent))
        with QPainter(pixmap) as painter:
            painter.setRenderHint(QPainter.Antialiasing)
            painter.setWindow(0, 0, pixmap.width(), pixmap.height())

            # Qt coordinate system is top down, we invert here
            painter.scale(1, -1)
            painter.translate(0, -pixmap.height())

            # check if we have no alarm or warning set complete
            if None in alarm.values() and None in warning.values():
                w_value = self.drawEmpty(painter)

            # check if we have both alarm and warnings
            elif not (None in alarm.values() or None in warning.values()):
                w_value = self.drawFull(painter, warning, alarm, value)

            # check if we only have warnings
            elif None in alarm.values():
                w_value = self.drawOnlyWarnings(painter, warning, value)

            # check if we only have alarms
            elif None in warning.values():
                w_value = self.drawOnlyAlarms(painter, alarm, value)

            # Draw surrounding rect
            painter.setPen(QPen(Qt.black, 1, Qt.SolidLine))
            painter.drawRect(MARGIN, 0, pixmap.width() - 2 * MARGIN,
                             pixmap.height())

            # Finally, draw the triangles
            H_TRI = 0.04  # Half of the triangle height
            path = QPainterPath()

            # right triangle
            path.moveTo(self._width / 2, w_value * self._height)
            path.lineTo(self._width, (w_value + H_TRI) * self._height)
            path.lineTo(self._width, (w_value - H_TRI) * self._height)
            path.lineTo(self._width / 2, w_value * self._height)

            # left triangle
            path.lineTo(0, (w_value + H_TRI) * self._height)
            path.lineTo(0, (w_value - H_TRI) * self._height)
            path.lineTo(self._width / 2, w_value * self._height)
            painter.fillPath(path, Qt.black)

            # We also have to draw the lines for a nicer shape
            painter.drawPath(path)
        self.widget.setPixmap(pixmap)

    def valueChanged(self, box, value, timestamp=None):
        desc = box.descriptor
        warning = {"LOW": desc.warnLow,
                   "HIGH": desc.warnHigh}
        alarm = {"LOW": desc.alarmLow,
                 "HIGH": desc.alarmHigh}
        if value != self.value:
            self.paintWidget(value, warning, alarm)
            self.value = value
