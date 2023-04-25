# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pyqtgraph import GraphicsLayoutWidget

from karabogui.graph.common.api import AuxPlots
from karabogui.testing import GuiTestCase

from ..controller import AGGREGATOR_MAP, AuxPlotsController


class TestAuxPlots(GuiTestCase):

    def setUp(self):
        super(TestAuxPlots, self).setUp()
        self._image_layout = GraphicsLayoutWidget()
        self._controller = AuxPlotsController(image_layout=self._image_layout)

    def test_basics(self):
        controllers = self._controller._aggregators
        self.assertEqual(self._controller.current_plot, AuxPlots.NoPlot)
        self.assertEqual(controllers, {})

        # Add aux plot controllers
        for aux_plot, klass in AGGREGATOR_MAP.items():
            self._controller.add(aux_plot)
            self.assertIn(aux_plot, controllers)
            self.assertIsInstance(controllers[aux_plot], klass)

        self.assertEqual(self._controller.current_plot, AuxPlots.NoPlot)

        layout_plots = set(self._image_layout.ci.items.keys())
        self.assertEqual(len(layout_plots), 0)

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
        self.assertTrue(len(self._image_layout.ci.items) == 0)

    def _assert_plots(self, *, shown):
        layout_plots = self._image_layout.ci.items.keys()

        for plot_type, aggregator in self._controller._aggregators.items():
            assertion = (self.assertIn if plot_type == shown
                         else self.assertNotIn)
            for plot in aggregator.controllers.values():
                assertion(plot.plotItem, layout_plots)
