from PyQt5.QtCore import pyqtSlot
from PyQt5.QtWidgets import QAction, QMenu

from karabogui.graph.image.utils import beam_profile_table_html

from .plot import StepPlot
from .profiler import IntensityProfiler

from ..base import BaseAuxPlotController


class ProfilePlotController(BaseAuxPlotController):
    """Beam profiling is done on an image by averaging the intensity of each
       axis. This results of needing two plots"""
    CONFIG = {"smooth": False}

    def __init__(self, **config):
        super(ProfilePlotController, self).__init__(**config)
        self._fitted = False

        # Add context menu
        menu = QMenu()
        enable_fit_action = QAction("Gaussian fitting", menu)
        enable_fit_action.setCheckable(True)
        enable_fit_action.triggered.connect(self._enable_fitting)
        menu.addAction(enable_fit_action)

        # Initialize plots
        self._profilers = []
        for orientation in ['top', 'left']:  # corresponding to (x, y)
            plot = StepPlot(orientation)
            plot.vb.set_menu(menu)
            profiler = IntensityProfiler(**self.config)
            self._plots.append((plot, profiler))

    def analyze(self, region):
        # Set axis to process
        for axis, (plot, profiler) in enumerate(self._plots):
            # Check if region wrt to the axis is valid
            if not region.valid(axis):
                plot.clear_data()
                profiler.clear_data()
                continue

            profiles = profiler.profile(region, axis=axis)
            if profiles is None:
                continue
            plot.set_data(*profiles)

            if self._fitted:
                plot.set_superimposed_data(*profiler.fit())

    def set_axes(self, x_data, y_data):
        """Set axes data to respective profilers"""
        self.analyzers[0].set_axis(x_data)
        self.analyzers[1].set_axis(y_data)

    def get_html(self):
        html = ''
        if self._stats_enabled:
            x_stats = self.analyzers[0].get_stats()
            y_stats = self.analyzers[1].get_stats()
            html = beam_profile_table_html(x_stats, y_stats)
        return html

    def link_viewbox(self, viewbox):
        for ax, plot in enumerate(self.plots):
            plot.vb.linkView(ax, viewbox)

    # -----------------------------------------------------------------------
    # Private methods

    @pyqtSlot(bool)
    def _enable_fitting(self, enabled):
        # For now, stats are also enabled when fitting is enabled
        self._stats_enabled = self._fitted = enabled

        # Process the changes
        if enabled:
            for plot, analyzer in self._plots:
                plot.set_superimposed_data(*analyzer.fit())
            stats = self.get_html()
        else:
            for plot in self.plots:
                plot.set_superimposed_data([], [])
            stats = ''

        self.showStatsRequested.emit(stats)
