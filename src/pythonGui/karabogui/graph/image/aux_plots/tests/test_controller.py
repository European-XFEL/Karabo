from pyqtgraph import GraphicsLayoutWidget

from karabogui.testing import GuiTestCase
from karabogui.graph.common.api import AuxPlots

from ..controller import (
    AuxPlotsController, HistogramController, ProfilePlotController)


class TestCase(GuiTestCase):

    def setUp(self):
        super().setUp()
        self._image_layout = GraphicsLayoutWidget()
        self._controller = AuxPlotsController(self._image_layout)

    def tearDown(self):
        self._controller.destroy()
        super(TestCase, self).tearDown()

    def test_basics(self):
        self._controller.add_from_type(AuxPlots.ProfilePlot)
        self._controller.add_from_type(AuxPlots.Histogram)

        self.assertIsNone(self._controller.current_plot)

        layout_plots = set(self._image_layout.ci.items.keys())
        self.assertEquals(len(layout_plots), 1)
        self.assertIn(self._controller.labelItem, layout_plots)

        # Show profile plot first
        self._controller.show(AuxPlots.ProfilePlot)
        self.assertIsInstance(self._controller.current_plot,
                              ProfilePlotController)
        self._assert_plots(shown=AuxPlots.ProfilePlot)

        # Show histogram next
        self._controller.show(AuxPlots.Histogram)
        self.assertIsInstance(self._controller.current_plot,
                              HistogramController)
        self._assert_plots(shown=AuxPlots.Histogram)

        # Don't show anything
        self._controller.show(AuxPlots.NoPlot)
        self.assertIsNone(self._controller.current_plot)
        self._assert_plots(shown=AuxPlots.NoPlot)

        # Clear controller
        self._controller.clear()
        self.assertTrue(len(self._image_layout.ci.items) == 0)
        self.assertIsNone(self._controller.current_plot)

    def _assert_plots(self, *, shown):
        layout_plots = self._image_layout.ci.items.keys()

        for plot_type, controller in self._controller._controllers.items():
            assertion = (self.assertIn if plot_type == shown
                         else self.assertNotIn)
            for plot in controller.plots:
                assertion(plot, layout_plots)
