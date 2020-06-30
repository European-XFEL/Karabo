import weakref

import numpy as np
from PyQt5.QtCore import QRectF
from PyQt5.QtGui import QPainter, QPainterPath, QPicture, QTransform
from PyQt5.QtWidgets import QGraphicsPathItem
from pyqtgraph import (
    arrayToQPath, BarGraphItem, functions as fn, getConfigOption,
    ScatterPlotItem)

from karabogui.graph.common.api import make_brush


class VectorFillGraphPlot(QGraphicsPathItem):
    """QGraphicsPathItem filling the space between a zero baseline and data
    """

    def __init__(self, viewbox=None, brush=None, pen=None):
        super(VectorFillGraphPlot, self).__init__()
        self._viewBox = weakref.ref(viewbox)
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
        self._viewBox().itemBoundsChanged(self)

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

        # Add the logMode in the options
        self.opts['logMode'] = [False, False]

    def getData(self):
        """Returns the corrected x and y data from the bar attributes."""
        # Calculate x-data from x
        x_data = self.opts['x']
        if self.opts['logMode'][0]:
            x_data = np.log10(x_data)

        # Calculate y-data from y0 and height
        height, y0 = self.opts['height'], self.opts['y0']
        y_data = np.array(height) - y0
        if self.opts['logMode'][1]:
            y_data = np.log10(y_data)

        return x_data, y_data

    def setData(self, x, y):
        self.setOpts(x=x, height=y, y0=0)

    def set_width(self, value):
        self.setOpts(width=value)

    # -----------------------------------------------------------------------
    # Qt methods

    def boundingRect(self):
        if self._shape is None:
            self.drawPicture()
        return self._shape.boundingRect()

    # -----------------------------------------------------------------------
    # pyqtgraph methods

    def drawPicture(self):
        """Reimplemented pyqtgraph method to modify values calculation"""
        self.picture = QPicture()
        self._shape = QPainterPath()
        p = QPainter(self.picture)

        pen = self.opts['pen']
        pens = self.opts['pens']

        if pen is None and pens is None:
            pen = getConfigOption('foreground')

        brush = self.opts['brush']
        brushes = self.opts['brushes']
        if brush is None and brushes is None:
            brush = (128, 128, 128)

        def asarray(x):
            if x is None or np.isscalar(x) or isinstance(x, np.ndarray):
                return x
            return np.array(x)

        # !----- Start modification -----!

        # MOD: Utilize corrected x- and y-data for the bar geometries
        x_data, y_data = self.getData()

        # MOD: Utilize x-data for the x0 calculation
        x = asarray(x_data)
        x0 = asarray(self.opts.get('x0'))
        width = asarray(self.opts.get('width'))

        # MOD: Calculate x0 based on x-data
        if x0 is None:
            if width is None:
                raise Exception('must specify either x0 or width')
            # MOD: Remove x1 calculation
            if x is not None:
                x0 = x - width / 2.
            else:
                # MOD: Remove x1 requirement on exception message
                raise Exception('must specify either x0 or width')
        # MOD: Remove width calculation

        # MOD: Remove y and y1 requirements
        y0 = asarray(self.opts.get('y0'))
        # MOD: Use the corrected y-data instead of height
        height = asarray(y_data)

        # MOD: Use a relative starting point and values if log-y is enabled
        if self.opts['logMode'][1] and height.size:
            order = np.floor(min(height))
            y0 = order - 1
            height -= y0

        # !----- End modification -----!

        p.setPen(fn.mkPen(pen))
        p.setBrush(fn.mkBrush(brush))
        for i in range(len(x0 if not np.isscalar(x0) else y0)):
            if pens is not None:
                p.setPen(fn.mkPen(pens[i]))
            if brushes is not None:
                p.setBrush(fn.mkBrush(brushes[i]))

            if np.isscalar(x0):
                x = x0
            else:
                x = x0[i]
            if np.isscalar(y0):
                y = y0
            else:
                y = y0[i]
            if np.isscalar(width):
                w = width
            else:
                w = width[i]
            if np.isscalar(height):
                h = height
            else:
                h = height[i]

            rect = QRectF(x, y, w, h)
            p.drawRect(rect)
            self._shape.addRect(rect)

        p.end()
        self.prepareGeometryChange()

    def setLogMode(self, xMode, yMode):
        """Stores the log mode. This is called by the viewbox."""
        log_mode = [xMode, yMode]
        if self.opts['logMode'] == log_mode:
            return
        self.setOpts(logMode=log_mode)


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
