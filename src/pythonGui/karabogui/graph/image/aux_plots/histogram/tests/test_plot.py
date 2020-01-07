import numpy as np
from PyQt5.QtCore import QRectF
from PyQt5.QtGui import (
    QBrush, QColor, QLinearGradient, QPainter, QPen, QPixmap)

from karabogui.testing import GuiTestCase
from karabogui.graph.common.api import COLORMAPS

from ..plot import HistogramPlot

VIRIDIS = "viridis"
LENGTH = 100
LEVELS = (0, 255)


class TestHistogramPlot(GuiTestCase):

    def setUp(self):
        super(TestHistogramPlot, self).setUp()
        self._plot = HistogramPlot()
        self._plot.set_colormap(VIRIDIS)
        self._colormap = COLORMAPS[VIRIDIS]

    def tearDown(self):
        super(TestHistogramPlot, self).tearDown()
        self._plot.deleteLater()

    def test_basics(self):
        self.assertEqual(self._plot._levels, LEVELS)
        self.assertIsInstance(self._plot._gradient, QLinearGradient)
        self.assertIsInstance(self._plot._pen, QPen)
        self.assertTrue(len(self._plot._data_items), 1)
        self.assertTrue(len(self._plot.vb.menu.actions()), 1)

    def test_set_colormap(self):
        cmap = "magma"
        self._plot.set_colormap(cmap)
        stops = [(stop, color.getRgb()[:3])
                 for stop, color in self._plot._gradient.stops()]
        self.assertListEqual(stops, COLORMAPS[cmap])

    def test_set_data(self):
        # 1. Set data equal to levels
        self._assert_gradient(start_value=0, stop_value=255)

        # 2. Set data inside levels range
        self._assert_gradient(start_value=50, stop_value=200)

        # 3. Set data outside levels range
        self._assert_gradient(start_value=-50, stop_value=300)

    def _assert_gradient(self, *, start_value, stop_value):
        x_data = np.linspace(start_value, stop_value, LENGTH)
        self._plot.set_data(x_data, np.arange(LENGTH - 1))

        # Get the index with respect to levels
        start_index = int(max(start_value, LEVELS[0]))
        stop_index = int(min(stop_value, LEVELS[1]))

        start_color, stop_color = self._get_border_colors()

        np.testing.assert_allclose(start_color, self._colormap[start_index][1],
                                   atol=3)
        np.testing.assert_allclose(stop_color, self._colormap[stop_index][1],
                                   atol=3)

    def _get_border_colors(self):
        """Get gradient start and final stop colors by painting its QPixmap
           and getting the color values from its QImage equivalent."""
        size = (1000, 10)
        brush = QBrush(self._plot._gradient)
        pixmap = QPixmap(*size)

        painter = QPainter(pixmap)
        painter.fillRect(QRectF(0, 0, *size), brush)
        painter.end()

        qimage = pixmap.toImage()
        start_color = QColor(qimage.pixel(1, 0)).getRgb()[:3]
        stop_color = QColor(qimage.pixel(999, 0)).getRgb()[:3]

        return start_color, stop_color
