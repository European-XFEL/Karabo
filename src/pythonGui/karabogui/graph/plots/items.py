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
from pyqtgraph import BarGraphItem, ScatterPlotItem

from karabogui.graph.common.api import make_brush, safe_log10


class VectorBarGraphPlot(BarGraphItem):
    """VectorBarGraphPlot Item Wrapper for a standard interface
    """

    def __init__(self, width, brush):
        X_DEFAULT, Y_DEFAULT = [], []
        super().__init__(
            x=X_DEFAULT, width=width, height=Y_DEFAULT, y0=0, brush=brush)

    def setOpts(self, **opts):
        if "logMode" not in self.opts:
            self.opts["logMode"] = [False, False]
        super().setOpts(**opts)

    def getData(self):
        """Returns the corrected x and y data from the bar attributes."""
        # Calculate x-data from x
        x_data = np.array(self.opts['x'])
        if self.opts['logMode'][0]:
            x_data = safe_log10(x_data)

        # Calculate y-data from y0 and height
        height, y0 = self.opts['height'], self.opts['y0']
        y_data = np.array(height) - y0
        if self.opts['logMode'][1]:
            y_data = safe_log10(y_data)

        return x_data, y_data

    def setData(self, x, y):
        self.setOpts(x=x, height=y, y0=0)

    def set_width(self, value):
        self.setOpts(width=value)

    # -----------------------------------------------------------------------
    # Qt methods

    def _getNormalizedCoords(self):
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

        # MOD: Use a relative starting point and values if log-y is enabled.
        # We consider values < 0 as nans
        if self.opts['logMode'][1] and height.size:
            order = np.floor(np.nanmin(height))
            y0 = order - 1
            height -= y0
            # Set height of invalid values as 0
            height[~np.isfinite(height)] = 0

        # !----- End modification -----!
        # ensure x0 < x1 and y0 < y1
        t0, t1 = x0, x0 + width
        x0 = np.minimum(t0, t1, dtype=np.float64)
        x1 = np.maximum(t0, t1, dtype=np.float64)
        t0, t1 = y0, y0 + height
        y0 = np.minimum(t0, t1, dtype=np.float64)
        y1 = np.maximum(t0, t1, dtype=np.float64)

        # here, all of x0, y0, x1, y1 are numpy objects,
        # BUT could possibly be numpy scalars
        return x0, y0, x1, y1

    def setLogMode(self, xMode, yMode):
        """Stores the log mode. This is called by the viewbox."""
        log_mode = [xMode, yMode]
        if self.opts.get('logMode', None) == log_mode:
            return
        self.setOpts(logMode=log_mode)


class ScatterGraphPlot(ScatterPlotItem):
    """ScatterGraphPlot Item to display the points in a Cloud

    NOTE: The most recent value has a ``firebrick`` brush!
    """
    last_value_brush = make_brush('r', 200)
    points_brush = make_brush('b', 200)

    def __init__(self, pen, cycle, **kwargs):
        self._cycle = cycle
        default_x, default_y = [], []
        super().__init__(
            x=default_x, y=default_y, pxMode=True, pen=pen, **kwargs)

    def setData(self, x, y, *args, **kwargs):
        """Set the data of the scatter plot"""
        if len(x) != len(y):
            return

        if self._cycle:
            size = len(x)
            if not size:
                super().setData(
                    x, y, brush=self.points_brush, **kwargs)
                return

            brush = []
            if size > 1:
                brush.extend([self.points_brush] * (size - 1))
            brush.append(self.last_value_brush)
        else:
            brush = self.points_brush

        super().setData(x, y, brush=brush, **kwargs)
