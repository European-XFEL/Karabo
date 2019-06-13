from PyQt4.QtCore import QObject, pyqtSlot
from pyqtgraph import LabelItem

from karabogui.graph.common.enums import AuxPlots

from ..aux_plots.profile_plot import ProfilePlot
from ..utils import beam_profile_table_html


class AuxPlotsController(QObject):

    PLOT_MAP = {
        AuxPlots.ProfilePlot: ProfilePlot
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
        self._fitted = False
        self._visible = False

        # Bookkeeping of plots
        self._current_plot_type = None
        self.current_plots = []

        self.labelItem = LabelItem()

    # -----------------------------------------------------------------------
    # Public methods

    def add_from_type(self, plot_type):
        """Set the aux plots of specified plot type to the image layout"""
        # Remove set plots if any
        plot_klass = self.PLOT_MAP[plot_type]

        # Add label item for aux plots stats. This will be shown in the topleft
        if not self.current_plots:
            self._image_layout.addItem(self.labelItem, 0, 0)

        for orientation in ['top', 'left']:
            plot_item = plot_klass(orientation=orientation)
            plot_item.setVisible(self._visible)
            plot_item.enable_fit_action.triggered.connect(self._enable_fitting)

            self.current_plots.append(plot_item)
            self._add_to_layout(plot_item)

        self._current_plot_type = plot_type
        return self.current_plots

    def show(self, show=True):
        """Show aux plots of specified type"""
        for plot in self.current_plots:
            plot.setVisible(show)

        self.labelItem.setVisible(show)
        self._visible = show

        # Process the changes
        if show and self._region is not None:
            self.analyze(self._region)

    def destroy(self):
        """Delete all the items and the controller gracefully"""
        # Delete all plots
        self.clear()
        self.deleteLater()

    def clear(self):
        for plot in self.current_plots:
            self._remove_from_layout(plot)
            plot.deleteLater()

        self._remove_from_layout(self.labelItem)

    @pyqtSlot(object)
    def analyze(self, region):
        """Gather information from the ROI region for our auxiliar plots"""
        # Store region for reanalyzing
        self._region = region

        if not self._visible:
            return

        profiles = [plot.analyze(region) for plot
                    in self.current_plots]

        if self._fitted and region is not None:
            table = beam_profile_table_html(*profiles)
            self.labelItem.item.setHtml(table)

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

    @pyqtSlot(bool)
    def _enable_fitting(self, enabled):
        self._fitted = enabled
        for plot in self.current_plots:
            plot.enable_fit(enabled)

        # Process the changes
        if enabled:
            self.analyze(self._region)
        else:
            self.labelItem.setText('')

        # Update label item size to avoid painting bugs
        self.labelItem.resizeEvent(None)
