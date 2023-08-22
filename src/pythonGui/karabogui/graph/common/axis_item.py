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
from pyqtgraph import AxisItem as PgAxisItem, DateAxisItem
from qtpy.QtCore import Qt, Signal

from karabogui.fonts import get_qfont
from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, X_AXIS_HEIGHT, Y_AXIS_WIDTH, get_alarm_string,
    get_state_string)
from karabogui.graph.common.enums import AxisType


def get_axis_font(size):
    """Return an appropriate karabo font with a point size `size`"""
    font = f"Source Sans Pro,{size},-1,5,50,0,0,0,0,0"
    return get_qfont(font)


class AxisItem(PgAxisItem):
    axisDoubleClicked = Signal()

    axisStyle = {
        "autoExpandTextSpace": False,
        "tickTextWidth": 50,
        "tickTextHeight": 24,
        "tickFont": get_axis_font(size=10)}

    def __init__(self, orientation, showValues=True):
        """Base class for pretty axis items.

        :param orientation: Location of the axis item in the plot
            Choose from ["left", "right", "top", "bottom"]
        :param showValues: Set if it will show ticks and labels.
            Usually major axis has the ticks (bottom and left for normal plots)
        """
        super().__init__(orientation, showValues=showValues)
        self.enableAutoSIPrefix(False)
        if showValues:
            self.setStyle(**self.axisStyle)

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.axisDoubleClicked.emit()
            event.accept()
            return
        super().mouseDoubleClickEvent(event)

    def setLabel(self, text=None, units=None, unitPrefix=None, **args):
        """Reimplemented because we don't want labels for minor axes"""
        if not self.style["showValues"]:
            return
        super().setLabel(text=text, units=units, unitPrefix=unitPrefix, **args)

    def showLabel(self, show=True):
        """Always do not show label if it is not a tick axis"""
        if not self.style["showValues"]:
            show = False
        super().showLabel(show)

    def mouseDragEvent(self, event):
        """Reimplemented function of PyQt

        PyQtGraph axis items have drag events which can enables panning
        of the viewbox.
        This is a problem for axis with grids because it is resized to fill the
        viewbox, which in turn will catch the wanted viewbox drag events.

        We ignore this event instead as it's not really used.
        """
        event.ignore()


class TimeAxisItem(DateAxisItem):
    axisDoubleClicked = Signal()

    axisStyle = {
        "autoExpandTextSpace": False,
        "tickTextWidth": 50,
        "tickTextHeight": 24,
        "tickFont": get_axis_font(size=10)}

    def __init__(self, orientation):
        super().__init__(orientation)
        self.setStyle(**self.axisStyle)

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.axisDoubleClicked.emit()
            event.accept()
            return
        super().mouseDoubleClickEvent(event)

    def mouseDragEvent(self, event):
        """Reimplemented function of PyQt"""
        event.ignore()

    def mouseClickEvent(self, event):
        """Reimplemented function of PyQt"""
        event.ignore()


class StateAxisItem(AxisItem):
    """The State Axis Item for displaying Karabo States as major ticks"""

    axisStyle = {"autoExpandTextSpace": True,
                 "tickFont": get_axis_font(size=9)}

    def tickStrings(self, values, scale, spacing):
        """Return the state names as a function of integers values"""
        return [get_state_string(value) for value in values]


class AlarmAxisItem(AxisItem):
    """The Alarm Axis Item for displaying Karabo Alarms as major ticks"""

    axisStyle = {"autoExpandTextSpace": True,
                 "tickFont": get_axis_font(size=9)}

    def tickStrings(self, values, scale, spacing):
        """Return the alarm names as a function of integers values"""
        return [get_alarm_string(value) for value in values]


class AuxPlotAxisItem(AxisItem):
    """The AxisItem for the aux plots in the image widgets"""
    axisStyle = {"autoExpandTextSpace": False,
                 "tickFont": get_axis_font(size=9)}

    def __init__(self, orientation, showValues=True):
        super().__init__(orientation, showValues)

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def _updateWidth(self):
        """Reimplemented function to match known width of main plot"""
        if self.style["showValues"]:
            self.setFixedWidth(Y_AXIS_WIDTH)
            self.picture = None
        else:
            super()._updateWidth()

    def _updateHeight(self):
        """Reimplemented function to match known height of main plot"""
        if self.style["showValues"]:
            height = X_AXIS_HEIGHT
            # Reduce space between label and axis for x-axis
            if self.orientation == "bottom":
                height -= 10
            self.setFixedHeight(height)
            self.picture = None
        else:
            super()._updateHeight()


def create_axis_items(axis=AxisType.Classic, axes_with_ticks=[]):
    """Create the basic axis items for graph plots

    The ``axis`` type defines the possible axis.
    """

    def _intersect(axis, tick_axis):
        return set(axis).intersection(tick_axis)

    time_axis, state_axis, aux_axis, alarm_axis = [], [], [], []

    if axis is AxisType.Time:
        time_axis = _intersect(AXIS_X, axes_with_ticks)
    elif axis is AxisType.State:
        state_axis = _intersect(AXIS_Y, axes_with_ticks)
        time_axis = _intersect(AXIS_X, axes_with_ticks)
    elif axis is AxisType.Alarm:
        alarm_axis = _intersect(AXIS_Y, axes_with_ticks)
        time_axis = _intersect(AXIS_X, axes_with_ticks)
    elif axis is AxisType.AuxPlot:
        aux_axis = axes_with_ticks
    items = {}
    for orientation in AXIS_ITEMS:
        has_ticks = orientation in axes_with_ticks

        if orientation in time_axis:
            axis_item = TimeAxisItem(orientation)
        elif orientation in state_axis:
            axis_item = StateAxisItem(orientation)
            axis_item.setTickSpacing(major=1, minor=1)
        elif orientation in alarm_axis:
            axis_item = AlarmAxisItem(orientation)
            axis_item.setTickSpacing(major=1, minor=1)
        elif orientation in aux_axis:
            axis_item = AuxPlotAxisItem(orientation, has_ticks)
        else:
            axis_item = AxisItem(orientation, has_ticks)

        items[orientation] = axis_item

    return items
