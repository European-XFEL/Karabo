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
from traits.api import Array

from ..base.analyzer import BaseAnalyzer


class HistogramAnalyzer(BaseAnalyzer):

    # The calculated edges of the input array
    _edges = Array

    # The counts of the edges from the input array
    _hist = Array

    # -----------------------------------------------------------------------
    # Public methods

    def analyze(self, region, **config):
        """Calculates the histogram of a 1D array."""
        self._hist, self._edges = np.histogram(region.flatten(), bins="sqrt")
        return self._edges, self._hist

    def clear_data(self):
        """Clear the previously calculated histogram and edges"""
        self._hist = np.array([])
        self._edges = np.array([])

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_stats(self):
        """Calculates and histogram statistics"""
        stats = {}
        if self._hist.size and self._edges.size:
            # Get the bin centers to use for mean and std calculations
            centers = (self._edges[1:] + self._edges[:-1]) / 2

            stats["count"] = self._hist.sum()
            stats["mean"] = np.average(centers, weights=self._hist)
            stats["mode"] = self._edges[self._hist.argmax()]
            stats["min"] = self._edges[0]
            stats["max"] = self._edges[-1]
            stats["std"] = np.sqrt(np.average((centers - stats["mean"]) ** 2,
                                              weights=self._hist))
            stats["bins"] = len(self._edges)
            stats["bin_width"] = self._edges[1] - self._edges[0]

        return stats
