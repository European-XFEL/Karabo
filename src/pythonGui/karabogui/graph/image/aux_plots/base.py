from PyQt5.QtCore import pyqtSignal, QObject


class BaseAuxPlotController(QObject):

    showStatsRequested = pyqtSignal(str)
    CONFIG = {}

    def __init__(self, **config):
        super(BaseAuxPlotController, self).__init__()

        # Variables
        self.config = self.CONFIG.copy()
        self.config.update(config)

        self._plots = []
        self._stats_enabled = False

    @property
    def plots(self):
        return [plot for plot, _ in self._plots]

    @property
    def analyzers(self):
        return [analyzer for _, analyzer in self._plots]

    def set_axes(self, x_data, y_data):
        """Subclass to use the transformed image axes as needed by the aux
           plots. For instance, this is used by the profiling as the x-data
           for each axis."""

    def analyze(self, region):
        """Subclass to perform calculations specific to the aux plots"""

    def get_html(self):
        """Subclass to render a report (usually statistics) that can be
           displayed in the companion label item, which is located at the
           top-left corner of the widget."""
        return ''

    def link_viewbox(self, viewbox):
        """Subclass to link the viewbox of the image plotItem depending on the
           aux plotItems"""


class BaseAuxPlotAnalyzer:

    def analyze(self, data):
        """Subclass to perform analysis specific to the aux plots"""

    def get_stats(self):
        """Subclass to perform statistics specific to the aux plots"""

    def set_axis(self, axis):
        """Subclass to use transformed image axes specific to the aux plots"""

    def clear_data(self):
        """Subclass to clear stored data based on the schema"""
