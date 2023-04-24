# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pyqtgraph import LabelItem, mkBrush, mkPen
from pyqtgraph.graphicsItems.LegendItem import ItemSample, LegendItem
from qtpy.QtCore import Qt
from qtpy.QtGui import QColor

SYMBOL_COLUMN = 0
TEXT_COLUMN = 1

TEXT_SIZE = "8pt"
LEGEND_PEN = mkPen(QColor(192, 192, 192, 200))
LEGEND_BRUSH = mkBrush(QColor(0, 0, 0, 50))
LABEL_COLOR = QColor(0, 0, 0, 200)


class KaraboLegend(LegendItem):
    def __init__(self, size=None, offset=None, **kwargs):
        super(KaraboLegend, self).__init__(
            size, offset, pen=LEGEND_PEN, brush=LEGEND_BRUSH,
            labelTextColor=LABEL_COLOR)

    def addItem(self, item, name):
        """Reimplemented function of LegendItem

        :param item: A PlotDataItem from which the line and point style
                     of the item will be determined
        :param name: The title to display for this item. Simple HTML allowed.
        """
        label = LabelItem(name, justify='left',
                          color=self.opts['labelTextColor'],
                          size=TEXT_SIZE)
        sample = (item if isinstance(item, ItemSample)
                  else ColorBox(item))
        row = self.layout.rowCount()
        self.items.append((sample, label))
        self.layout.addItem(sample, row, SYMBOL_COLUMN)
        self.layout.addItem(label, row, TEXT_COLUMN)
        self.updateSize()


class CoordsLegend(KaraboLegend):
    def __init__(self, color='w'):
        super(CoordsLegend, self).__init__()
        self._label = LabelItem(color=color, size="8pt")
        self.layout.addItem(self._label, 0, 0)
        self.layout.setContentsMargins(2, 2, 2, 2)
        self.hide()

    def set_value(self, x, y):
        self._label.setText("x: {}<br>y: {}".format(x, y))


class ColorBox(ItemSample):
    """The color box in the legend that shows the curve pen color"""

    def __init__(self, item):
        super(ColorBox, self).__init__(item)
        self._pen = mkPen(item.opts.get('pen', None))
        self._brush = mkBrush(self._pen.color())

    def paint(self, painter, *args):
        painter.setPen(self._pen)
        painter.setBrush(self._brush)
        painter.drawRect(0, 0, 10, 14)

    def setBrush(self, brush):
        self._brush = brush
        self.update()

    def mouseClickEvent(self, event):
        """Reimplemented function of PyQt

        Use the mouse click with the left button to toggle the visibility
        of the plot curve item as well as to change the appearance of the box
        """
        if event.button() == Qt.LeftButton:
            visible = self.item.isVisible()
            self.item.setVisible(not visible)
            color = QColor(211, 211, 211) if visible else self._pen.color()
            brush = mkBrush(color)
            self.setBrush(brush)

        event.accept()
