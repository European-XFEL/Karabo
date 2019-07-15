from PyQt4.QtGui import QBrush, QColor, QPen
from pyqtgraph import GraphicsWidget, LabelItem, mkBrush, mkPen
from pyqtgraph.graphicsItems.LegendItem import ItemSample, LegendItem

SYMBOL_COLUMN = 0
TEXT_COLUMN = 1


class KaraboLegend(LegendItem):
    def __init__(self, size=None, offset=None):
        super(KaraboLegend, self).__init__(size, offset)
        self._pen = QPen(QColor(192, 192, 192, 200), 1)
        self._pen.setCosmetic(True)
        self._brush = QBrush(QColor(0, 0, 0, 50))

    def addItem(self, item, name):
        """Reimplemented function of LegendItem

        :param item: A PlotDataItem from which the line and point style
                     of the item will be determined
        :param name: The title to display for this item. Simple HTML allowed.
        """
        label = LabelItem(name, justify='left', color='w', size="8pt")
        sample = (item if isinstance(item, ItemSample)
                  else ColorBox(item))
        row = self.layout.rowCount()
        self.items.append((sample, label))
        self.layout.addItem(sample, row, SYMBOL_COLUMN)
        self.layout.addItem(label, row, TEXT_COLUMN)
        self.updateSize()

    def paint(self, painter, *args):
        painter.setPen(self._pen)
        painter.setBrush(self._brush)
        rect = self.boundingRect()
        painter.drawRoundedRect(rect, 2, 2)


class CoordsLegend(KaraboLegend):
    def __init__(self):
        super(CoordsLegend, self).__init__()
        self._label = LabelItem(color='w', size="8pt")
        self.layout.addItem(self._label, 0, 0)
        self.layout.setContentsMargins(2, 2, 2, 2)
        self.hide()

    def set_value(self, x, y):
        if x is None or y is None:
            self._label.setText("x: {}<br>y: {}".format(x, y))
        else:
            self._label.setText("x: {:.2f}<br>y: {:.2f}".format(x, y))


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
