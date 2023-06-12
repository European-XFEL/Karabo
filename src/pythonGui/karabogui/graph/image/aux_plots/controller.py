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
from pyqtgraph import GraphicsLayoutWidget, LabelItem
from traits.api import Dict, Enum, HasStrictTraits, Instance

from karabogui.graph.common.api import AuxPlots, ImageRegion

from .histogram.controller import HistogramAggregator
from .profiling.controller import ProfileAggregator

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
    labelItem = Instance(LabelItem, kw={"justify": "left"})

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
        if orientation == "top":
            self.image_layout.ci.layout.setRowStretchFactor(row, 1)
            self.image_layout.ci.layout.setRowMinimumHeight(row, 100)
        elif orientation == "left":
            self.image_layout.ci.layout.setColumnStretchFactor(col, 1)
            self.image_layout.ci.layout.setColumnMinimumWidth(col, 100)

    def _remove_item(self, item, orientation=None):
        if item in self.image_layout.ci.items.keys():
            self.image_layout.removeItem(item)
        # Collapse the grid space by removing the stretch factor of the
        # emptied space
        row, col = GRID_ORIENTATION[orientation]
        if orientation == "top":
            self.image_layout.ci.layout.setRowStretchFactor(row, 0)
            self.image_layout.ci.layout.setRowMinimumHeight(row, 0)
        elif orientation == "left":
            self.image_layout.ci.layout.setColumnStretchFactor(col, 0)
            self.image_layout.ci.layout.setColumnMinimumWidth(col, 0)

    def _show_stats(self, stats):
        """Add the labelItem only if there's stats shown"""
        label_added = self.labelItem in self.image_layout.ci.items.keys()
        if stats is not None:
            if not label_added:
                # Span the label on unoccupied orientation.
                # The item spans by 1 row and column by default,
                # but now we span to an unoccupied space if available.
                # For instance, with histogram aux plot, that does not have a
                # left plot, we span the label by two columns.
                aggregator = self._aggregators[self.current_plot]
                plot_orientations = aggregator.controllers.keys()
                row_span = 1 + ("left" not in plot_orientations)
                col_span = 1 + ("top" not in plot_orientations)
                self.image_layout.addItem(self.labelItem, 0, 0,
                                          row_span, col_span)
            self.labelItem.item.setHtml(stats.html)
        elif label_added:
            self.labelItem.setText('')
            self.image_layout.removeItem(self.labelItem)
        self.labelItem.resizeEvent(None)

    # -----------------------------------------------------------------------
    # Trait handlers

    def _current_plot_changed(self, old, new):
        # Check if requested plot is in instantiated aggregators
        if new not in self._aggregators:
            new = AuxPlots.NoPlot

        # Remove previous plot items
        if old != AuxPlots.NoPlot:
            aggregator = self._aggregators[old]
            for orientation, plot in aggregator.controllers.items():
                self._remove_item(plot.plotItem, orientation)
            self._show_stats(None)

        if new != AuxPlots.NoPlot:
            # Add new plot items
            aggregator = self._aggregators[new]
            for orientation, plot in aggregator.controllers.items():
                self._add_item(plot.plotItem, orientation)
            self._show_stats(None)

        # Process the changes
        if self._region is not None:
            self.process(self._region)
