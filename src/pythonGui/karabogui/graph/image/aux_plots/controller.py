from PyQt5.QtCore import QObject, pyqtSlot
from pyqtgraph import LabelItem

from karabogui.graph.common.api import AuxPlots

from .profiling.controller import ProfilePlotController
from .histogram.controller import HistogramController


class AuxPlotsController(QObject):

    PLOT_MAP = {
        AuxPlots.ProfilePlot: ProfilePlotController,
        AuxPlots.Histogram: HistogramController
    }

    def __init__(self, image_layout):
        """Controller for showing/hiding aux plots, switching between
        different classes,

        :param plot_type: A class classes of desired aux plots.
           e.g.: klass = ProfilePlot, Histogram
        """
        super(AuxPlotsController, self).__init__()

        self._image_layout = image_layout
        self._region = None

        # Bookkeeping of plots
        self.current_plot = None
        self._controllers = {}

        self.labelItem = LabelItem()
        self._image_layout.addItem(self.labelItem, 0, 0)

    # -----------------------------------------------------------------------
    # Public methods

    def add_from_type(self, plot_type, **config):
        """Set the aux plots of specified plot type to the image layout"""
        # Remove set plots if any
        plot_klass = self.PLOT_MAP[plot_type]

        # Add label item for aux plots stats. This will be shown in the topleft
        if plot_type in self._controllers:
            return

        controller = plot_klass(**config)
        controller.showStatsRequested.connect(self._show_stats)

        self._controllers[plot_type] = controller

        return controller

    def show(self, plot_type):
        """Show aux plots of specified type"""

        # Check if requested plot is in instantiated controllers
        if plot_type not in self._controllers:
            plot_type = AuxPlots.NoPlot

        # Set visibility of label item and previous plots, if any
        self.labelItem.setVisible(plot_type != AuxPlots.NoPlot)
        if self.current_plot is not None:
            for plot in self.current_plot.plots:
                self._remove_from_layout(plot)

        # Return intermittently if plot type is None
        if plot_type is AuxPlots.NoPlot:
            self.current_plot = None
            return

        # Add plots to layout
        controller = self._controllers[plot_type]
        for plot in controller.plots:
            self._add_to_layout(plot)

        # Process the changes
        if self._region is not None:
            self.analyze(self._region)

        self.current_plot = controller
        self.labelItem.setText('')  # clear contents

    def destroy(self):
        """Delete all the items and the controller gracefully"""
        # Delete all plots
        self.clear()
        self.deleteLater()

    def clear(self):
        # Check if already cleared
        if not len(self._image_layout.ci.items):
            return

        if self.current_plot is not None:
            for plot in self.current_plot.plots:
                self._remove_from_layout(plot)
                plot.deleteLater()

        self._remove_from_layout(self.labelItem)
        self.current_plot = None

    @pyqtSlot(object)
    def analyze(self, region):
        """Gather information from the ROI region for our auxiliar plots"""
        # Store region for reanalyzing
        self._region = region

        if self.current_plot is None:
            return

        self.current_plot.analyze(region)
        stats = self.current_plot.get_html()
        self._show_stats(stats, update=False)

    def set_image_axes(self, axes):
        for controller in self._controllers.values():
            controller.set_axes(x_data=axes[0], y_data=axes[1])

    def set_config(self, plot=None, **config):
        controller = self._controllers.get(plot)
        if controller is None:
            return

        if plot is AuxPlots.Histogram:
            if 'colormap' in config:
                controller.set_colormap(config['colormap'])
            if 'levels' in config:
                controller.set_levels(config['levels'])

    # -----------------------------------------------------------------------
    # Private methods

    def _add_to_layout(self, plot_item):
        if plot_item.orientation == "left":
            row, col = (1, 0)
        elif plot_item.orientation == "top":
            row, col = (0, 1)
        else:  # "bottom", "right"
            raise NotImplementedError

        self._image_layout.addItem(plot_item, row, col)

    def _remove_from_layout(self, plot_item):
        self._image_layout.removeItem(plot_item)

    @pyqtSlot(str)
    def _show_stats(self, stats, update=True):
        self.labelItem.item.setHtml(stats)

        # Update label item size to avoid painting bugs
        if update:
            self.labelItem.resizeEvent(None)
