from PyQt4.QtGui import QFont
from pyqtgraph import AxisItem as PgAxisItem

from karabogui.graph.common.const import AXIS_ITEMS


class AxisItem(PgAxisItem):

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


def get_axis_items(axes_with_ticks, klass=AxisItem):
    items = {}
    for orientation in AXIS_ITEMS:
        has_ticks = orientation in axes_with_ticks
        items[orientation] = klass(orientation, has_ticks)
    return items
