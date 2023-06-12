# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from pyqtgraph import PlotDataItem
from qtpy.QtCore import QPointF, Qt
from qtpy.QtGui import QBrush, QColor, QLinearGradient, QPen
from traits.api import (
    Array, Constant, Instance, Property, String, cached_property,
    on_trait_change)

from karabogui.graph.common.api import COLORMAPS
from karabogui.graph.image.utils import rescale

from ..base.plot import BasePlot


class HistogramPlot(BasePlot):

    # The x- and y-labels of the plot
    x_label = Constant("Intensity")
    y_label = Constant("Counts")

    # Image levels
    levels = Array(value=[0, 255])

    # Colormap of the
    colormap = String("viridis")

    # --- plot items ---
    # The line item of the plot
    _data_item = Instance(PlotDataItem)

    # --- UI attributes ---
    # The default pen of the plot.
    _pen = Instance(QPen, args=(Qt.NoPen,))

    # The gradient of the brush to fill the area under the curve
    gradient = Property(Instance(QLinearGradient), depends_on='colormap')

    def __init__(self, **traits):
        super().__init__(**traits)
        self._data_item = self._add_plot_item()

    # -----------------------------------------------------------------------
    # Public methods

    def set_data(self, x_data, y_data):
        # Retrieve the gradient range from the input x_data and image levels
        brush = self._calc_brush(x_data)

        # plot the data with the resulting gradient
        self._data_item.setData(x_data, y_data, stepMode="center",
                                pen=self._pen, brush=brush, fillLevel=0)

    def destroy(self):
        super().destroy()
        self.on_trait_change(self._set_brush, "levels", remove=True)

    # -----------------------------------------------------------------------
    # Private methods

    def _calc_brush(self, x_data):
        start, stop = rescale(self.levels,
                              min_value=x_data[0],
                              max_value=x_data[-1],
                              low=0, high=1)

        # Create the brush by modifying the gradient ranges
        gradient = self.gradient
        gradient.setStart(round(start, 3), 0)
        gradient.setFinalStop(round(stop, 3), 0)
        return QBrush(gradient)

    # -----------------------------------------------------------------------
    # Trait handlers

    @on_trait_change("levels")
    def _set_brush(self):
        # Get brush with the new levels and from existing data
        x_data = self._data_item.xData
        if x_data is not None and x_data.size:
            brush = self._calc_brush(x_data)
            self._data_item.setBrush(brush)

    # -----------------------------------------------------------------------
    # Trait properties

    @cached_property
    def _get_gradient(self):
        gradient = QLinearGradient(QPointF(0, 0), QPointF(1, 0))
        gradient.setCoordinateMode(QLinearGradient.ObjectBoundingMode)
        grad_stops = [(stop, QColor(*color))
                      for stop, color in COLORMAPS[self.colormap]]
        gradient.setStops(grad_stops)
        return gradient
