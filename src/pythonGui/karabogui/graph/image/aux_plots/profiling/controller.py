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
from traits.api import (
    ArrayOrNone, Bool, Dict, Instance, Property, Type, on_trait_change)

from ..base.controller import BaseController, ControllerAggregator
from .analyzer import ProfileAnalyzer
from .plot import ProfilePlot
from .stats import ProfileStats


class ProfileController(BaseController):

    # The analyzer class for the profile plot
    analyzer = Instance(ProfileAnalyzer, args=())

    # The plot item controller class for the profile plot
    plot_klass = Type(ProfilePlot)

    # Fit the profile data. This is toggled by a context menu.
    fitted = Bool(False)

    # Transformed image axis data. From the analyzer class
    axis = Property(ArrayOrNone)

    # Smoothen the resulting signal. From the analyzer class.
    smooth = Property

    # -----------------------------------------------------------------------
    # Public methods

    def process(self, region, axis=0):
        """Process is made up of two parts:
           1. Analyze the data first
           2. Plot the data"""

        # Clean up first
        self.clear_data()

        profiles = self.analyzer.analyze(region, axis=axis)
        if profiles is None:
            return
        self.plot.set_data(*profiles)
        if self.fitted:
            x_fit, y_fit = self.analyzer.fit()
            self.plot.set_fit(x_fit, y_fit)

    # -----------------------------------------------------------------------
    # Trait handlers

    def _fitted_changed(self, fitted):
        x_fit, y_fit = [], []
        if fitted:
            x_fit, y_fit = self.analyzer.fit()
        self.plot.set_fit(x_fit, y_fit)

    # -----------------------------------------------------------------------
    # Trait properties

    def _set_axis(self, axis):
        self.analyzer.axis_data = axis

    def _get_smooth(self):
        return self.analyzer.smooth

    def _set_smooth(self, is_smooth):
        self.analyzer.smooth = is_smooth


class ProfileAggregator(ControllerAggregator):

    # The profile aux plot contains two profile plots
    contents = Dict({"top": ProfileController, "left": ProfileController})

    # Profile plot instances that are connected for x- and y-axis
    x_plot = Property(Instance(ProfileController))
    y_plot = Property(Instance(ProfileController))

    # --- Convenience properties to get/set values on the histogram plot ---
    # Smoothen the resulting signal. Retrieved from both profile plots
    smooth = Property

    def __init__(self, **traits):
        super().__init__(**traits)

        # Create shared menu
        menu = QMenu()
        enable_fit_action = QAction("Gaussian fitting", menu)
        enable_fit_action.setCheckable(True)
        enable_fit_action.setChecked(self.show_stats)
        enable_fit_action.triggered.connect(self._enable_fitting)
        menu.addAction(enable_fit_action)

        self.x_plot.plot.menu = menu
        self.y_plot.plot.menu = menu

    # -----------------------------------------------------------------------
    # Public methods

    def process(self, region):
        self.x_plot.process(region, axis=0)
        self.y_plot.process(region, axis=1)

        if self.show_stats:
            self.stats = ProfileStats(x_stats=self.x_plot.analyzer.stats,
                                      y_stats=self.y_plot.analyzer.stats)

    def link_view(self, view):
        """Connects the image plot viewbox to the respective aux plots"""
        self.x_plot.link_view(axis=0, view=view)
        self.y_plot.link_view(axis=1, view=view)

    def set_axes(self, x_data, y_data):
        self.x_plot.axis = x_data
        self.y_plot.axis = y_data

    def destroy(self):
        super().destroy()
        self.on_trait_change(self._show_stats, "show_stats", remove=True)

    # -----------------------------------------------------------------------
    # Private methods

    def _enable_fitting(self, enabled):
        self.x_plot.fitted = self.y_plot.fitted = enabled
        self.show_stats = enabled

    # -----------------------------------------------------------------------
    # Trait handlers

    @on_trait_change("show_stats")
    def _show_stats(self, show_stats):
        stats = None
        if show_stats:
            stats = ProfileStats(x_stats=self.x_plot.analyzer.stats,
                                 y_stats=self.y_plot.analyzer.stats)
        self.stats = stats

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_x_plot(self):
        return self.controllers["top"]

    def _get_y_plot(self):
        return self.controllers["left"]

    def _get_smooth(self):
        return self.x_plot.smooth and self.y_plot.smooth

    def _set_smooth(self, smooth):
        self.x_plot.smooth = smooth
        self.y_plot.smooth = smooth
