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
import numpy as np
from pyqtgraph import PlotDataItem
from qtpy.QtGui import QBrush, QPen
from traits.api import Constant, Instance

from karabogui.graph.common.api import make_brush, make_pen

from ..base.plot import BasePlot


class ProfilePlot(BasePlot):

    # The y-label of the plot
    y_label = Constant("Intensity")

    # --- plot items ---
    # The line item of the profile plot
    _data_item = Instance(PlotDataItem)

    # The line item of the fit plot
    _fit_item = Instance(PlotDataItem)

    # --- UI attributes ---
    # The default pen and brush of the profile data
    _data_color = 'b'
    _data_pen = Instance(QPen, factory=make_pen(_data_color))
    _data_brush = Instance(QBrush, factory=make_brush(_data_color, alpha=140))

    # The default pen of the fit data
    _fit_color = 'r'
    _fit_pen = Instance(QPen, factory=make_pen(_fit_color))

    def __init__(self, **traits):
        super().__init__(**traits)
        self._data_item = self._add_plot_item()
        self._fit_item = self._add_plot_item()

    # -----------------------------------------------------------------------
    # Public methods

    def set_data(self, x_data, y_data):
        # Assuming that data is discrete, we add the difference to the last
        # value and feed it to the plot. This last is needed to plot all values
        # when stepMode="center" (True), which is then not used.
        if x_data.size <= 1 or y_data.size <= 1:
            return

        offset = x_data[1] - x_data[0]
        x_corrected = np.append(x_data, x_data[-1] + offset)
        self._data_item.setData(x_corrected, y_data,
                                stepMode="center", pen=self._data_pen,
                                brush=self._data_brush, fillLevel=0)

        # Adjust the y range to the data range to not show the pedestal.
        y_range = ("yRange" if self.orientation in ["top", "bottom"]
                   else "xRange")

        data_range = (y_data.min(), y_data.max())
        # Both need to be finite!
        if all(np.isfinite(data_range)):
            self.plotItem.setRange(**{y_range: data_range})

    def set_fit(self, x_data, y_data):
        self._fit_item.setData(x_data, y_data, pen=self._fit_pen)
