import numpy as np
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont, QPen
from pyqtgraph import AxisItem as PgAxisItem, DateAxisItem

from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, X_AXIS_HEIGHT, Y_AXIS_WIDTH, INTEGER_ALARM_MAP,
    INTEGER_STATE_MAP)
from karabogui.graph.common.enums import AxisType

HOUR_IN_SECONDS = 60 * 60
DAY_IN_SECONDS = HOUR_IN_SECONDS * 24
MONTH_IN_SECONDS = DAY_IN_SECONDS * 30
YEAR_IN_SECONDS = MONTH_IN_SECONDS * 12

STEPS_BY_5 = np.array([1., 2., 10., 20., 100.])
STEPS_BY_4 = np.array([2.5, 5., 10., 25., 50., 100.])


class AxisItem(PgAxisItem):
    axisDoubleClicked = pyqtSignal()

    axisStyle = {
        "autoExpandTextSpace": False,
        "tickTextWidth": 50,
        "tickTextHeight": 24}

    tickFontSize = 11

    def __init__(self, orientation, showValues=True):
        """Base class for pretty axis items.

        :param orientation: Location of the axis item in the plot
            Choose from ["left", "right", "top", "bottom"]
        :param showValues: Set if it will show ticks and labels.
            Usually major axis has the ticks (bottom and left for normal plots)
        """

        super(AxisItem, self).__init__(orientation, showValues=showValues)
        self.enableAutoSIPrefix(False)
        # Modify tick aesthetics if major axis
        if showValues:
            self.setStyle(**self.axisStyle)
        else:
            # tick strings and labels are not shown
            self.fixedWidth = 0
            self.fixedHeight = 0
            self.setTextPen(QPen())

        font = QFont()
        font.setPixelSize(self.tickFontSize)
        self.setTickFont(font)

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.axisDoubleClicked.emit()
            event.accept()
            return
        super(AxisItem, self).mouseDoubleClickEvent(event)

    def setLabel(self, text=None, units=None, unitPrefix=None, **args):
        """Reimplemented because we don't want labels for minor axes"""
        if not self.style["showValues"]:
            return
        super(AxisItem, self).setLabel(text=text, units=units,
                                       unitPrefix=unitPrefix, **args)

    def showLabel(self, show=True):
        """Always do not show label if it is not a tick axis"""
        if not self.style["showValues"]:
            show = False
        super(AxisItem, self).showLabel(show)

    def mouseDragEvent(self, event):
        """Reimplemented function of PyQt

        PyQtGraph axis items have drag events which can enables panning
        of the viewbox.
        This is a problem for axis with grids because it is resized to fill the
        viewbox, which in turn will catch the wanted viewbox drag events.

        We ignore this event instead as it's not really used.
        """
        event.ignore()

    def tickSpacing(self, minVal, maxVal, size):
        """Reimplemented function of PyQtGraph

        The axis ticks calculation of pyqtgraph has a fixed frequency.
        As `generateDrawSpecs` has a lot of coupled functions, we resort to the
        easier fix of decreasing the frequency (from every 5 to every 4)
        depending on the size.
        """
        # First check for override tick spacing
        if self._tickSpacing is not None:
            return self._tickSpacing

        dif = abs(maxVal - minVal)
        if dif == 0:
            return []

        # decide optimal minor tick spacing in pixels (this is just aesthetics)
        optimalTickCount = max(2., np.log(size))

        # optimal minor tick spacing
        optimalSpacing = dif / optimalTickCount

        # the largest power-of-10 spacing which is smaller than optimal
        p10unit = 10 ** np.floor(np.log10(optimalSpacing))

        # Determine major/minor tick spacings which flank the optimal spacing.
        # !----- Start modification -----!
        steps = STEPS_BY_5 if size > 300 else STEPS_BY_4
        intervals = steps * p10unit
        # !----- End modification -----!
        minorIndex = 0
        while intervals[minorIndex + 1] <= optimalSpacing:
            minorIndex += 1

        levels = [
            (intervals[minorIndex + 2], 0),
            (intervals[minorIndex + 1], 0),
            # (intervals[minorIndex], 0)    ## Pretty, but eats up CPU
        ]

        if self.style['maxTickLevel'] >= 2:
            # decide whether to include the last level of ticks
            minSpacing = min(size / 20., 30.)
            maxTickCount = size / minSpacing
            if dif / intervals[minorIndex] <= maxTickCount:
                levels.append((intervals[minorIndex], 0))

        return levels


class TimeAxisItem(DateAxisItem):
    axisDoubleClicked = pyqtSignal()

    axisStyle = {
        "autoExpandTextSpace": False,
        "tickTextWidth": 50,
        "tickTextHeight": 24}

    tickFontSize = 10

    def __init__(self, orientation):
        super(TimeAxisItem, self).__init__(orientation)
        self.enableAutoSIPrefix(False)
        self.setStyle(**self.axisStyle)
        font = QFont()
        font.setPixelSize(self.tickFontSize)
        self.setTickFont(font)

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.axisDoubleClicked.emit()
            event.accept()
            return
        super(AxisItem, self).mouseDoubleClickEvent(event)

    def mouseDragEvent(self, event):
        """Reimplemented function of PyQt
        """
        event.ignore()


class StateAxisItem(AxisItem):
    """The State Axis Item for displaying Karabo States as major ticks"""

    axisStyle = {"autoExpandTextSpace": True}
    tickFontSize = 10

    def tickStrings(self, values, scale, spacing):
        """Return the state names as a function of integers values

        NOTE: Always cast the value as integer due to PyQtGraph protection!
        """
        return [INTEGER_STATE_MAP.get(int(value), '') for value in values]


class AlarmAxisItem(AxisItem):
    """The Alarm Axis Item for displaying Karabo Alarms as major ticks"""

    axisStyle = {"autoExpandTextSpace": True}
    tickFontSize = 10

    def tickStrings(self, values, scale, spacing):
        """Return the alarm names as a function of integers values

        NOTE: Always cast the value as integer due to PyQtGraph protection!
        """
        return [INTEGER_ALARM_MAP.get(int(value), '') for value in values]


class AuxPlotAxisItem(AxisItem):
    """The AxisItem for the aux plots in the image widgets"""
    tickFontSize = 9
    axisStyle = {"autoExpandTextSpace": False}

    def __init__(self, orientation, showValues=True):
        super(AuxPlotAxisItem, self).__init__(orientation, showValues)

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def _updateWidth(self):
        """Reimplemented function to match known width of main plot"""
        if self.style["showValues"]:
            self.setFixedWidth(Y_AXIS_WIDTH)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateWidth()

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
            super(AuxPlotAxisItem, self)._updateHeight()

    def labelString(self):
        """Reimplemented function to improve label display of scaled axes

        This is required for axis values ~ 10^6.
        """
        if self.labelUnits != '':
            units = ('(%s%s)') % (self.labelUnitPrefix, self.labelUnits)
        else:
            units = ''

        s = ('%s %s') % (self.labelText, units)
        style = ';'.join(['%s: %s' % (k, self.labelStyle[k])
                          for k in self.labelStyle])
        main = "<span align='center' style='%s'>%s</span>" % (style, s)

        if not self.autoSIPrefix or self.autoSIPrefixScale == 1.0:
            scaling = ''
        else:
            scaling = ('<center>(\u00D7 %g)</center>'
                       % (1.0 / self.autoSIPrefixScale))

        scale_style = []
        for k in self.labelStyle:
            if k == "font-size":
                font_size = int(self.labelStyle[k][:-2].strip()) - 2
                scale_style.append("%s: %s" % (k, str(font_size) + "px"))
            else:
                scale_style.append("%s: %s" % (k, self.labelStyle[k]))
        style = ';'.join(scale_style)
        scale = ("<span align='center' style='%s'>%s</span>"
                 % (style, scaling))

        return '%s %s' % (main, scale)


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
