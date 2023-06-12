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
from qtpy.QtWidgets import QAction, QMenu
from traits.api import Bool, Dict, Instance, Property, Type, on_trait_change

from ..base.controller import BaseController, ControllerAggregator
from .analyzer import HistogramAnalyzer
from .plot import HistogramPlot
from .stats import HistogramStats


class HistogramController(BaseController):

    # The analyzer class for the histogram plot
    analyzer = Instance(HistogramAnalyzer, args=())

    # The plot item controller class for the histogram plot
    plot_klass = Type(HistogramPlot)


class HistogramAggregator(ControllerAggregator):

    # The histogram aux plot only contains one histogram plot
    contents = Dict({"top": HistogramController})

    # --- Plot configuration ---
    # The stats are shown by default
    show_stats = Bool(True)

    # --- Convenience properties to get/set values on the histogram plot ---
    controller = Property
    colormap = Property
    levels = Property

    def __init__(self, **traits):
        super().__init__(**traits)

        # Create menu
        menu = QMenu()
        show_stats_action = QAction("Show statistics", menu)
        show_stats_action.setCheckable(True)
        show_stats_action.setChecked(self.show_stats)
        show_stats_action.triggered.connect(lambda x:
                                            self.trait_set(show_stats=x))
        menu.addAction(show_stats_action)
        self.controller.plot.menu = menu

    # -----------------------------------------------------------------------
    # Public methods

    def process(self, region):
        self.controller.process(region, axis=0)
        if self.show_stats:
            self.stats = HistogramStats(stats=self.controller.analyzer.stats)

    # -----------------------------------------------------------------------
    # Trait handlers

    @on_trait_change("show_stats")
    def _show_stats(self, show_stats):
        stats = None
        if show_stats:
            stats = HistogramStats(stats=self.controller.analyzer.stats)
        self.stats = stats

    def destroy(self):
        super().destroy()
        self.on_trait_change(self._show_stats, "show_stats", remove=True)

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_controller(self):
        return self.controllers["top"]

    def _set_colormap(self, colormap):
        self.controller.plot.colormap = colormap

    def _set_levels(self, levels):
        self.controller.plot.levels = levels
