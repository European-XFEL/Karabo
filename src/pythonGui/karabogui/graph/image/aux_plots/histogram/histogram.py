import numpy as np

from ..base import BaseAuxPlotAnalyzer


class Histogram(BaseAuxPlotAnalyzer):

    def __init__(self):
        self._hist = None
        self._edges = None

    def analyze(self, data):
        """Calculates the histogram of a 1D array."""
        self._hist, self._edges = np.histogram(data, bins="sqrt")

        return self._edges, self._hist

    def get_stats(self):
        if self._hist is None and self._edges is None:
            return

        # Get the bin centers to use for mean and std calculations
        centers = (self._edges[1:] + self._edges[:-1]) / 2

        stats = {}
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

    def clear_data(self):
        self._hist = None
        self._edges = None
