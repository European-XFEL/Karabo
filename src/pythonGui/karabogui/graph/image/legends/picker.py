from PyQt4.QtCore import Qt
from PyQt4.QtGui import QBrush, QPen, QColor
from pyqtgraph import LabelItem
from pyqtgraph.graphicsItems.LegendItem import ItemSample

from karabogui.graph.common.api import float_to_string, KaraboLegend


class PickerLegend(KaraboLegend):

    def __init__(self):
        super(PickerLegend, self).__init__()
        self.layout.setContentsMargins(2, 2, 2, 2)
        self.layout.setHorizontalSpacing(15)

        self._color_box = ColorSample()
        self.layout.addItem(self._color_box, 0, 0)

        self._label = LabelItem(color='w', size="8pt", justify="left")
        self.layout.addItem(self._label, 0, 1)

        self.layout.setColumnFixedWidth(0, self._color_box.width())
        self.layout.setRowAlignment(0, Qt.AlignVCenter)
        self.layout.setColumnAlignment(1, Qt.AlignLeft)
        self.hide()

    def set_value(self, x, y, value, color):
        x, y, value = [float_to_string(num) for num in [x, y, value]]
        self._label.setText("x: {}<br>y: {}<br>value: {}".format(x, y, value))
        self._color_box.setBrush(QBrush(color))
        self.updateSize()

    def updateSize(self):
        if self.size is not None:
            return

        self.prepareGeometryChange()
        width = (self._color_box.width() + self._label.width())
        self.setGeometry(0, 0, width, self._label.height())


class ColorSample(ItemSample):
    """The color item in the legend that shows the selected pixel color"""

    def __init__(self):
        super(ItemSample, self).__init__(None)
        self._brush = Qt.NoBrush
        self._pen = QPen(QColor(196, 197, 193, 200), 1)

    def paint(self, painter, *args):
        painter.setPen(self._pen)
        painter.setBrush(self._brush)
        painter.drawRect(0, 0, 12, 45)

    def setBrush(self, brush):
        self._brush = brush
        self.update()
