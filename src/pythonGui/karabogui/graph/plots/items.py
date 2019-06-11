import numpy as np
from PyQt4.QtGui import QGraphicsPathItem, QPainterPath, QTransform
from pyqtgraph import arrayToQPath, BarGraphItem, ScatterPlotItem

from karabogui.graph.common.api import make_brush


class VectorFillGraphPlot(QGraphicsPathItem):
    """QGraphicsPathItem filling the space between a zero baseline and data
    """

    def __init__(self, brush=None, pen=None):
        super(VectorFillGraphPlot, self).__init__()
        if brush is not None:
            self.setBrush(brush)
        if pen is not None:
            self.setPen(pen)
        self.curves = None

    def setData(self, data):
        """Sets the data for the top and bottom curves"""
        size = len(data)
        zeros = np.zeros(size)
        baseline = np.arange(size)
        self.curves = [(baseline, zeros), (baseline, data)]
        self.updatePath()

    def updatePath(self):
        if self.curves is None:
            return
        paths = [arrayToQPath(x, y) for x, y in self.curves]
        transform = QTransform()
        sub_path_base = paths[0].toSubpathPolygons(transform)
        sub_path_data = paths[1].toReversed().toSubpathPolygons(transform)
        sub_path_data.reverse()
        path = QPainterPath()
        for base_ele, data_ele in zip(sub_path_base, sub_path_data):
            path.addPolygon(base_ele + data_ele)
        self.setPath(path)


class VectorBarGraphPlot(BarGraphItem):
    """VectorBarGraphPlot Item Wrapper for a standard interface
    """

    def __init__(self, width, brush):
        X_DEFAULT, Y_DEFAULT = [], []
        super(VectorBarGraphPlot, self).__init__(
            x=X_DEFAULT, width=width, height=Y_DEFAULT, y0=0, brush=brush)

    def setData(self, x, y):
        self.setOpts(x=x, height=y, y0=0)

    def set_width(self, value):
        self.setOpts(width=value)


class ScatterGraphPlot(ScatterPlotItem):
    """ScatterGraphPlot Item to display the points in a Cloud

    NOTE: The most recent value has a ``firebrick`` brush!
    """
    _brushes = {
        0: make_brush('r'),  # last value brush
        1: make_brush('b'),  # all other points
    }

    def __init__(self, pen, cycle, **kwargs):
        self._cycle = cycle
        X_DEFAULT, Y_DEFAULT = [], []
        super(ScatterGraphPlot, self).__init__(x=X_DEFAULT, y=Y_DEFAULT,
                                               pen=pen, **kwargs)

    def setData(self, x, y, *args, **kwargs):
        """Set the data of the scatter plot"""
        if not len(x) == len(y):
            return

        if self._cycle:
            num_data = len(x)
            if not num_data:
                return

            brush = []
            if num_data > 1:
                brush.extend([self._brushes[1]] * (num_data - 1))
            brush.append(self._brushes[0])
        else:
            brush = self._brushes[1]

        super(ScatterGraphPlot, self).setData(x, y, brush=brush)
