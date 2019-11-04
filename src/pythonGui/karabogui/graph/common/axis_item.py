from datetime import datetime

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont
from pyqtgraph import AxisItem as PgAxisItem
from pyqtgraph.functions import siScale

from karabogui.graph.common.const import (
    AXIS_ITEMS, X_AXIS_HEIGHT, Y_AXIS_WIDTH, INTEGER_STATE_MAP)
from karabogui.graph.common.enums import AxisType


class AxisItem(PgAxisItem):
    axisDoubleClicked = pyqtSignal()

    def __init__(self, orientation, has_ticks=True):
        """
        Base class for pretty axis items.

        :param orientation: Location of the axis item in the plot
            Choose from ["left", "right", "top", "bottom"]
        :param has_ticks: Set if it will show ticks and labels.
            Usually major axis has the ticks (bottom and left for normal plots)
        """

        self.has_ticks = has_ticks
        super(AxisItem, self).__init__(orientation)

        # Modify tick aesthetics if major axis
        # (tick strings and labels are shown)
        if not has_ticks:
            self.fixedWidth = 0
            self.fixedHeight = 0

        font = QFont()
        font.setPixelSize(11)
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
        super(AxisItem, self).setLabel(text=text,
                                       units=units,
                                       unitPrefix=unitPrefix,
                                       **args)

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


class TimeAxisItem(AxisItem):
    LABEL_FORMATS = ((60 + 30, "%Ss"),  # Up to 1min 30s
                     (60 * 60 * 4, "%H:%M"),  # Up to 4 hours
                     (60 * 60 * 28, "%Hh"),  # Up to 1 day
                     (60 * 60 * 24 * 25, "%d"),  # Up to 25 days
                     (60 * 60 * 24 * 30 * 6, "%d/%m"))  # Up to 6 months

    def __init__(self, *args, **kwargs):
        super(TimeAxisItem, self).__init__(*args, **kwargs)

        self.format_string = '%Ss'

    def setRange(self, min_range, max_range):
        """Set the range and decide which format string we should use"""
        super(TimeAxisItem, self).setRange(min_range, max_range)

        first_datetime = datetime.utcfromtimestamp(min_range)
        last_datetime = datetime.utcfromtimestamp(max_range)

        # Get the whole diff in seconds
        diff_in_seconds = (last_datetime - first_datetime).total_seconds()
        for seconds_for_format, format_string in self.LABEL_FORMATS:
            if diff_in_seconds < seconds_for_format:
                self.format_string = format_string
                break
        else:
            self.format_string = '%d/%m/%Y'

    def tickStrings(self, values, scale, spacing):
        values = [datetime.fromtimestamp(value).strftime(self.format_string)
                  for value in values]
        return values


class StateAxisItem(AxisItem):
    """The State Axis Item for displaying Karabo States as major ticks"""

    def tickStrings(self, values, scale, spacing):
        """Return the state names as a function of integers values

        NOTE: Always cast the value as integer due to PyQtGraph protection!
        """
        return [INTEGER_STATE_MAP.get(int(value), 'UNKNOWN')
                for value in values]


class AuxPlotAxisItem(AxisItem):
    """AxisItem for the aux plots."""

    def __init__(self, orientation, has_ticks=True):
        super(AuxPlotAxisItem, self).__init__(orientation, has_ticks)
        self.setStyle(autoExpandTextSpace=False)
        font = QFont()
        font.setPixelSize(8)
        self.tickFont = font

    # ---------------------------------------------------------------------
    # pyqtgraph methods

    def _updateWidth(self):
        """Patching to match known width of main plot"""
        if self.label.isVisible():
            self.setFixedWidth(Y_AXIS_WIDTH)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateWidth()

    def _updateHeight(self):
        """Patching to match known height of main plot"""
        if self.label.isVisible():
            self.setFixedHeight(X_AXIS_HEIGHT)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateHeight()

    def updateAutoSIPrefix(self):
        """Patching to scale tick values after 10^3"""
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
        """Patching to improve label display of scaled axes (e.g. x 10^6)"""
        if self.labelUnits != '':
            units = ('(%s%s)') % ((self.labelUnitPrefix),
                                  (self.labelUnits))
        else:
            units = ''

        s = ('%s %s') % ((self.labelText), (units))
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
    time_axis = []
    state_axis = []
    aux_axis = []

    if axis is AxisType.Time:
        time_axis = ["top", "bottom"]
    elif axis is AxisType.State:
        state_axis = ["left"]
        time_axis = ["top", "bottom"]
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
        elif orientation in aux_axis:
            axis_item = AuxPlotAxisItem(orientation, has_ticks)
        else:
            axis_item = AxisItem(orientation, has_ticks)

        items[orientation] = axis_item

    return items
