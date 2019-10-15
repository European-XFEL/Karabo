from datetime import datetime

from PyQt4.QtCore import Qt, pyqtSignal
from PyQt4.QtGui import QFont
from pyqtgraph import AxisItem as PgAxisItem

from karabogui.graph.common import const


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


def create_axis_items(axes_with_ticks, time_axis=None, klass=AxisItem):
    items = {}

    for orientation in const.AXIS_ITEMS:
        has_ticks = orientation in axes_with_ticks

        if time_axis is not None and orientation in time_axis:
            axis_item = TimeAxisItem(orientation)
        else:
            axis_item = klass(orientation, has_ticks)

        items[orientation] = axis_item

    return items
