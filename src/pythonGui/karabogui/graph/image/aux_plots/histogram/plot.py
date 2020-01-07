from PyQt5.QtCore import QPointF, Qt
from PyQt5.QtGui import QBrush, QColor, QLinearGradient, QPen
from PyQt5.QtWidgets import QAction, QMenu

from karabogui.graph.common.api import COLORMAPS
from karabogui.graph.image.utils import rescale

from ..plot import BaseAuxPlotItem


class HistogramPlot(BaseAuxPlotItem):

    def __init__(self, orientation="top"):
        super(HistogramPlot, self).__init__(orientation,
                                            x_label="Intensity",
                                            y_label="Counts")

        self._levels = (0, 255)
        self._gradient = QLinearGradient()
        self._pen = QPen(Qt.NoPen)

        line = self.plot([], [])
        self._data_items = [line]
        self._adjust_to_orientation()

        # Add actions
        menu = QMenu()
        self.show_stats_action = QAction("Show statistics", menu)
        self.show_stats_action.setCheckable(True)
        self.show_stats_action.setChecked(True)
        menu.addAction(self.show_stats_action)
        self.vb.set_menu(menu)

    def set_data(self, x_data, y_data):
        start, stop = rescale(self._levels,
                              min_value=x_data[0],
                              max_value=x_data[-1],
                              low=0, high=1)
        self._gradient.setStart(round(start, 3), 0)
        self._gradient.setFinalStop(round(stop, 3), 0)

        self._data_items[0].setData(
            x_data, y_data,
            stepMode=True,
            pen=self._pen, brush=QBrush(self._gradient),
            fillLevel=0)

    def set_colormap(self, cmap):
        gradient = QLinearGradient(QPointF(0, 0), QPointF(1, 0))
        gradient.setCoordinateMode(QLinearGradient.ObjectBoundingMode)
        grad_stops = [(stop, QColor(*color))
                      for stop, color in COLORMAPS[cmap]]

        gradient.setStops(grad_stops)
        self._gradient = gradient

    def set_levels(self, levels):
        self._levels = levels
