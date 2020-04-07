from pyqtgraph import GraphicsLayoutWidget, LabelItem
from traits.api import Dict, Enum, HasStrictTraits, Instance

from karabogui.graph.common.api import AuxPlots, ImageRegion

from .profiling.controller import ProfileAggregator
from .histogram.controller import HistogramAggregator


# Map of the enum and the aux plot aggregator
AGGREGATOR_MAP = {
    AuxPlots.ProfilePlot: ProfileAggregator,
    AuxPlots.Histogram: HistogramAggregator
}

# Grid orientation of the image layout, where the center is (1, 1)
GRID_ORIENTATION = {
    "top": (0, 1),
    "left": (1, 0),
}


class AuxPlotsController(HasStrictTraits):
    """This is used by the image base to control the AuxPlots as one entity.
       This contains the aggregators of each added type, which, in turn,
       contains the controllers of each plot contents. This also has the
       shared labelItem that shows the statistics in an HTML format."""

    # The image layout that contains the image plot and will house the
    # aux plot items
    image_layout = Instance(GraphicsLayoutWidget)

    # The region from the image plot
    _region = Instance(ImageRegion)

    # The current aux plot. Default value is `AuxPlots.NoPlot`
    current_plot = Enum(*AuxPlots)

    # The added aggregators: e.g., `{AuxPlots.Histogram: HistogramAuxPlot()}`
    _aggregators = Dict

    # The UI item that shows HTML tables of the statistics
    labelItem = Instance(LabelItem)

    def __init__(self, **traits):
        super(AuxPlotsController, self).__init__(**traits)

        # Set up widgets
        self.labelItem = LabelItem()
        self.image_layout.addItem(self.labelItem, 0, 0)

    # -----------------------------------------------------------------------
    # Public methods

    def add(self, plot_type, **config):
        """Set the aux plots of specified plot type to the image layout"""
        # Add label item for aux plots stats. This will be shown in the topleft
        if plot_type in self._aggregators:
            return

        # Remove set plots if any
        aggregator_klass = AGGREGATOR_MAP[plot_type]
        aggregator = aggregator_klass(config=config)
        aggregator.on_trait_change(self._show_stats, 'stats')

        self._aggregators[plot_type] = aggregator
        return aggregator

    def destroy(self):
        """Delete all the items and the controller gracefully"""
        # Check if already cleared
        if not len(self.image_layout.ci.items):
            return

        self.image_layout.clear()
        for aggregator in self._aggregators.values():
            aggregator.destroy()

        self.labelItem.deleteLater()

    def process(self, region):
        self._region = region
        if self.current_plot is not AuxPlots.NoPlot:
            aux_plot = self._aggregators.get(self.current_plot)
            aux_plot.process(region)

    def set_axes(self, x_data, y_data):
        for aux_plot in self._aggregators.values():
            aux_plot.set_axes(x_data, y_data)

    def set_config(self, plot=None, **config):
        aggregator = self._aggregators.get(plot)
        if aggregator is not None:
            aggregator.trait_set(**config)

    # -----------------------------------------------------------------------
    # Private methods

    def _add_item(self, plot_item, orientation="top"):
        row, col = GRID_ORIENTATION[orientation]
        self.image_layout.addItem(plot_item, row, col)

    def _remove_item(self, item):
        if item in self.image_layout.ci.items.keys():
            self.image_layout.removeItem(item)

    def _show_stats(self, stats):
        if stats is not None:
            self.labelItem.item.setHtml(stats.html)
            self.labelItem.resizeEvent(None)
        else:
            # Clear labelItem
            self.labelItem.setText('')

    # -----------------------------------------------------------------------
    # Trait handlers

    def _current_plot_changed(self, old, new):
        # Check if requested plot is in instantiated aggregators
        if new not in self._aggregators:
            new = AuxPlots.NoPlot

        # Set visibility of label item and previous plots, if any
        self.labelItem.setVisible(new != AuxPlots.NoPlot)

        # Remove previous plot items
        if old is not AuxPlots.NoPlot:
            for plot in self._aggregators[old].plotItems:
                self._remove_item(plot)

        if new is not AuxPlots.NoPlot:
            # Add new plot items
            aggregator = self._aggregators[new]
            for orientation, plot in aggregator.controllers.items():
                self._add_item(plot.plotItem, orientation)

        # Clear contents
        self.labelItem.setText('')

        # Process the changes
        if self._region is not None:
            self.process(self._region)
