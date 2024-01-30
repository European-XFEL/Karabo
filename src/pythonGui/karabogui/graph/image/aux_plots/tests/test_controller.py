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
from pyqtgraph import GraphicsLayoutWidget

from karabogui.graph.common.api import AuxPlots
from karabogui.testing import GuiTestCase

from ..controller import AGGREGATOR_MAP, AuxPlotsController


class TestAuxPlots(GuiTestCase):

    def setUp(self):
        super().setUp()
        self._image_layout = GraphicsLayoutWidget()
        self._controller = AuxPlotsController(image_layout=self._image_layout)

    def test_basics(self):
        controllers = self._controller._aggregators
        assert self._controller.current_plot == AuxPlots.NoPlot
        assert controllers == {}

        # Add aux plot controllers
        for aux_plot, klass in AGGREGATOR_MAP.items():
            self._controller.add(aux_plot)
            assert aux_plot in controllers
            assert isinstance(controllers[aux_plot], klass)

        assert self._controller.current_plot == AuxPlots.NoPlot

        layout_plots = set(self._image_layout.ci.items.keys())
        assert len(layout_plots) == 0

        # Show profile plot first
        self._controller.current_plot = AuxPlots.ProfilePlot
        self._assert_plots(shown=AuxPlots.ProfilePlot)

        # Show histogram next
        self._controller.current_plot = AuxPlots.Histogram
        self._assert_plots(shown=AuxPlots.Histogram)

        # Don't show anything
        self._controller.current_plot = AuxPlots.NoPlot
        self._assert_plots(shown=AuxPlots.NoPlot)

        # Clear controller
        self._controller.destroy()
        assert len(self._image_layout.ci.items) == 0

    def _assert_plots(self, *, shown):
        layout_plots = self._image_layout.ci.items.keys()

        for plot_type, aggregator in self._controller._aggregators.items():
            assertion = (self.assertIn if plot_type == shown
                         else self.assertNotIn)
            for plot in aggregator.controllers.values():
                assertion(plot.plotItem, layout_plots)
