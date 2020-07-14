from datetime import datetime

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont
from pyqtgraph import AxisItem as PgAxisItem
from pyqtgraph.functions import siScale

from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, X_AXIS_HEIGHT, Y_AXIS_WIDTH, INTEGER_ALARM_MAP,
    INTEGER_STATE_MAP)
from karabogui.graph.common.enums import AxisType

HOUR_IN_SECONDS = 60 * 60
DAY_IN_SECONDS = HOUR_IN_SECONDS * 24
MONTH_IN_SECONDS = DAY_IN_SECONDS * 30
YEAR_IN_SECONDS = MONTH_IN_SECONDS * 12


class AxisItem(PgAxisItem):
    axisDoubleClicked = pyqtSignal()

    STYLE = {
        "autoExpandTextSpace": False,
        "tickTextWidth": 50,
        "tickTextHeight": 24
    }

    FONT_SIZE = 10

    def __init__(self, orientation, has_ticks=True):
        """Base class for pretty axis items.

        :param orientation: Location of the axis item in the plot
            Choose from ["left", "right", "top", "bottom"]
        :param has_ticks: Set if it will show ticks and labels.
            Usually major axis has the ticks (bottom and left for normal plots)
        """

        self.has_ticks = has_ticks
        super(AxisItem, self).__init__(orientation)
        self.enableAutoSIPrefix(False)
        # Modify tick aesthetics if major axis
        # (tick strings and labels are shown)
        if has_ticks:
            self.setStyle(**self.STYLE)
        else:
            self.fixedWidth = 0
            self.fixedHeight = 0

        font = QFont()
        font.setPixelSize(self.FONT_SIZE)
        self.tickFont = font

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.axisDoubleClicked.emit()
            event.accept()
            return
        super(AxisItem, self).mouseDoubleClickEvent(event)

    def setLabel(self, text=None, units=None, unitPrefix=None, **args):
        """Reimplemented because we don't want labels for minor axes"""
        if not self.has_ticks:
            return
        super(AxisItem, self).setLabel(text=text, units=units,
                                       unitPrefix=unitPrefix, **args)

    def showLabel(self, show=True):
        # Always do not show label if it is not a tick axis
        if not self.has_ticks:
            show = False
        super(AxisItem, self).showLabel(show)

    def drawPicture(self, p, axisSpec, tickSpecs, textSpecs):
        """Reimplemented because we don't want tick strings for minor axes"""
        p.setRenderHint(p.Antialiasing, False)
        p.setRenderHint(p.TextAntialiasing, True)

        # draw long line along axis
        pen, p1, p2 = axisSpec
        p.setPen(pen)
        p.drawLine(p1, p2)
        p.translate(0.5, 0)  # resolves some damn pixel ambiguity

        # draw ticks
        for pen, p1, p2 in tickSpecs:
            p.setPen(pen)
            p.drawLine(p1, p2)

        if self.has_ticks:
            # Draw all text
            if self.tickFont is not None:
                p.setFont(self.tickFont)
            p.setPen(self.pen())
            for rect, flags, text in textSpecs:
                p.drawText(rect, flags, text)

    def mouseDragEvent(self, event):
        """Reimplemented function of PyQt

        PyQtGraph axis items have drag events which can enables panning
        of the viewbox.
        This is a problem for axis with grids because it is resized to fill the
        viewbox, which in turn will catch the wanted viewbox drag events.

        We ignore this event instead as it's not really used.
        """
        event.ignore()


class TimeAxisItem(AxisItem):
    format_string = "%Ss"

    def setRange(self, min_range, max_range):
        """Set the range and decide which format string we should use"""
        first = datetime.utcfromtimestamp(min_range)
        last = datetime.utcfromtimestamp(max_range)
        diff = (last - first).total_seconds()

        datetime_format = "%b %Y"

        if last.year == first.year:
            datetime_format = "%d %b"
            if last.day == first.day:
                datetime_format = "%H:%M"
                if diff < 60 * 10:  # Difference is under 10 mins
                    datetime_format = "%H:%M:%S"
            elif diff < DAY_IN_SECONDS * 7:
                datetime_format = "%d %b, %H:%M"
        elif diff < YEAR_IN_SECONDS * 3:
            datetime_format = "%d %b %Y"

        self.format_string = datetime_format
        super(TimeAxisItem, self).setRange(min_range, max_range)

    def tickStrings(self, values, scale, spacing):
        values = [datetime.fromtimestamp(value).strftime(self.format_string)
                  for value in values]
        return values


class StateAxisItem(AxisItem):
    """The State Axis Item for displaying Karabo States as major ticks"""

    STYLE = {"autoExpandTextSpace": True}
    FONT_SIZE = 9

    def tickStrings(self, values, scale, spacing):
        """Return the state names as a function of integers values

        NOTE: Always cast the value as integer due to PyQtGraph protection!
        """
        return [INTEGER_STATE_MAP.get(int(value), '') for value in values]


class AlarmAxisItem(AxisItem):
    """The Alarm Axis Item for displaying Karabo Alarms as major ticks"""

    STYLE = {"autoExpandTextSpace": True}
    FONT_SIZE = 9

    def tickStrings(self, values, scale, spacing):
        """Return the alarm names as a function of integers values

        NOTE: Always cast the value as integer due to PyQtGraph protection!
        """
        return [INTEGER_ALARM_MAP.get(int(value), '') for value in values]


class AuxPlotAxisItem(AxisItem):
    """AxisItem for the aux plots."""

    def __init__(self, orientation, has_ticks=True):
        super(AuxPlotAxisItem, self).__init__(orientation, has_ticks)
        self.setStyle(autoExpandTextSpace=False)
        font = QFont()
        font.setPixelSize(8)
        self.tickFont = font

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def _updateWidth(self):
        """Reimplemented function to match known width of main plot"""
        if self.has_ticks:
            self.setFixedWidth(Y_AXIS_WIDTH)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateWidth()

    def _updateHeight(self):
        """Reimplemented function to match known height of main plot"""
        if self.has_ticks:
            height = X_AXIS_HEIGHT
            # Reduce space between label and axis for x-axis
            if self.orientation == "bottom":
                height -= 10
            self.setFixedHeight(height)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateHeight()

    def updateAutoSIPrefix(self):
        """Reimplemented function to scale tick values after ~10^3"""
        if self.label.isVisible():
            (scale, prefix) = siScale(max(abs(self.range[0] * self.scale),
                                          abs(self.range[1] * self.scale)))
            if self.labelUnits == '' and prefix == 'k':
                scale = 1.0
                prefix = ''
            self.setLabel(unitPrefix=prefix)
        else:
            scale = 1.0

        self.autoSIPrefixScale = scale
        self.picture = None
        self.update()

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
