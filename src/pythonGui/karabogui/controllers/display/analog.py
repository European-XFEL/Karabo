#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 5, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import Qt, QRectF
from PyQt4.QtGui import QColor, QLabel, QPainter, QPainterPath, QPixmap, QPen
from traits.api import Instance

from karabo.common.scenemodel.api import AnalogModel
from karabogui import messagebox
from karabogui.alarms.api import ALARM_COLOR, NORM_COLOR, WARN_COLOR
from karabogui.binding.api import (
    FloatBinding, KARABO_ALARM_HIGH, KARABO_ALARM_LOW, KARABO_WARN_HIGH,
    KARABO_WARN_LOW)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)

B_ALOW, B_WLOW, B_WHIGH, B_AHIGH = (0.1, 0.3, 0.7, 0.9)
DARKER_GREY = (64, 64, 64)
DARK_GREY = (125, 135, 135)
GREY = (200, 200, 200)

MARGIN = 5
WIDGET_HEIGHT = 180
WIDGET_WIDTH = 26


@register_binding_controller(ui_name='Analog Widget',
                             klassname='DisplayAnalog',
                             binding_type=FloatBinding)
class DisplayAnalog(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(AnalogModel, args=())

    def binding_update(self, proxy):
        alarms, warnings = self._alarmsAndWarnings()
        if None in alarms.values() and None in warnings.values():
            msg = ('No proper configuration detected for property "{}".\n'
                   'Please define alarm and warning thresholds.')
            messagebox.show_warning(msg.format(proxy.path),
                                    title='Wrong property configuration')

    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setAlignment(Qt.AlignCenter)
        widget.setFixedSize(WIDGET_WIDTH, WIDGET_HEIGHT)
        return widget

    def value_update(self, proxy):
        alarms, warnings = self._alarmsAndWarnings()
        if alarms and warnings:
            self._paintWidget(proxy.value, warnings, alarms)

    # -------------------------------------------------------------------------
    # Private

    def _alarmsAndWarnings(self):
        if self.proxy.binding is None:
            return {}, {}

        attributes = self.proxy.binding.attributes
        warnings = {"LOW": attributes.get(KARABO_WARN_LOW),
                    "HIGH": attributes.get(KARABO_WARN_HIGH)}
        alarms = {"LOW": attributes.get(KARABO_ALARM_LOW),
                  "HIGH": attributes.get(KARABO_ALARM_HIGH)}
        return alarms, warnings

    def _drawRegion(self, painter, start, stop, color):
        rect = QRectF(MARGIN, start * WIDGET_HEIGHT,
                      WIDGET_WIDTH - 2 * MARGIN,
                      (stop - start) * WIDGET_HEIGHT)
        painter.fillRect(rect, QColor(*color))

    def _drawEmpty(self, painter):
        self._drawRegion(painter, B_AHIGH, 1, DARKER_GREY)
        self._drawRegion(painter, B_WHIGH, B_AHIGH, DARK_GREY)
        self._drawRegion(painter, B_WLOW, B_WHIGH, GREY)
        self._drawRegion(painter, B_ALOW, B_WLOW, DARK_GREY)
        self._drawRegion(painter, 0, B_ALOW, DARKER_GREY)
        w_value = 0.5

        return w_value

    def _drawOnlyAlarms(self, painter, alarm, value):
        self._drawRegion(painter, B_AHIGH, 1, ALARM_COLOR)
        self._drawRegion(painter, B_ALOW, B_AHIGH, NORM_COLOR)
        self._drawRegion(painter, 0, B_ALOW, ALARM_COLOR)

        if value >= alarm["LOW"] and value <= alarm["HIGH"]:
            max_min = alarm["HIGH"] - alarm["LOW"]
            norm_region = (value - alarm["LOW"]) / max_min
            w_value = norm_region * (B_AHIGH - B_ALOW) + B_ALOW
        elif value > alarm["HIGH"]:
            w_value = (1 - B_AHIGH) / 2 + B_AHIGH
        elif value < alarm["LOW"]:
            w_value = B_ALOW / 2

        return w_value

    def _drawOnlyWarnings(self, painter, warning, value):
        self._drawRegion(painter, B_WHIGH, 1, WARN_COLOR)
        self._drawRegion(painter, B_WLOW, B_WHIGH, NORM_COLOR)
        self._drawRegion(painter, 0, B_WLOW, WARN_COLOR)

        if value >= warning["LOW"] and value <= warning["HIGH"]:
            max_min = warning["HIGH"] - warning["LOW"]
            norm_region = (value - warning["LOW"]) / max_min
            w_value = norm_region * (B_WHIGH - B_WLOW) + B_WLOW
        elif value > warning["HIGH"]:
            w_value = (1 - B_WHIGH) / 2 + B_WHIGH
        elif value < warning["LOW"]:
            w_value = B_WLOW / 2

        return w_value

    def _drawFull(self, painter, warning, alarm, value):
        self._drawRegion(painter, B_AHIGH, 1, ALARM_COLOR)
        self._drawRegion(painter, B_WHIGH, B_AHIGH, WARN_COLOR)
        self._drawRegion(painter, B_WLOW, B_WHIGH, NORM_COLOR)
        self._drawRegion(painter, B_ALOW, B_WLOW, WARN_COLOR)
        self._drawRegion(painter, 0, B_ALOW, ALARM_COLOR)

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

    def _paintWidget(self, value, warning, alarm):
        pixmap = QPixmap(WIDGET_WIDTH, WIDGET_HEIGHT)
        pixmap.fill(QColor(Qt.transparent))
        with QPainter(pixmap) as painter:
            painter.setRenderHint(QPainter.Antialiasing)
            painter.setWindow(0, 0, pixmap.width(), pixmap.height())

            # Qt coordinate system is top down, we invert here
            painter.scale(1, -1)
            painter.translate(0, -pixmap.height())

            # check if we have no alarm or warning set complete
            if None in alarm.values() and None in warning.values():
                w_value = self._drawEmpty(painter)

            # check if we have both alarm and warnings
            elif not (None in alarm.values() or None in warning.values()):
                w_value = self._drawFull(painter, warning, alarm, value)

            # check if we only have warnings
            elif None in alarm.values():
                w_value = self._drawOnlyWarnings(painter, warning, value)

            # check if we only have alarms
            elif None in warning.values():
                w_value = self._drawOnlyAlarms(painter, alarm, value)

            # Draw surrounding rect
            painter.setPen(QPen(Qt.black, 1, Qt.SolidLine))
            painter.drawRect(MARGIN, 0, pixmap.width() - 2 * MARGIN,
                             pixmap.height())

            # Finally, draw the triangles
            H_TRI = 0.04  # Half of the triangle height
            path = QPainterPath()

            # right triangle
            path.moveTo(WIDGET_WIDTH / 2, w_value * WIDGET_HEIGHT)
            path.lineTo(WIDGET_WIDTH, (w_value + H_TRI) * WIDGET_HEIGHT)
            path.lineTo(WIDGET_WIDTH, (w_value - H_TRI) * WIDGET_HEIGHT)
            path.lineTo(WIDGET_WIDTH / 2, w_value * WIDGET_HEIGHT)

            # left triangle
            path.lineTo(0, (w_value + H_TRI) * WIDGET_HEIGHT)
            path.lineTo(0, (w_value - H_TRI) * WIDGET_HEIGHT)
            path.lineTo(WIDGET_WIDTH / 2, w_value * WIDGET_HEIGHT)
            painter.fillPath(path, Qt.black)

            # We also have to draw the lines for a nicer shape
            painter.drawPath(path)
        self.widget.setPixmap(pixmap)
