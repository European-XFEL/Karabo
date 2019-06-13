from pyqtgraph import GraphicsLayoutWidget

from karabogui.testing import GuiTestCase

from karabogui.graph.image.api import AuxPlotsController
from karabogui.graph.common.enums import AuxPlots


class TestCase(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.image_layout = GraphicsLayoutWidget()

    def test_basic(self):
        assert not self.image_layout.ci.items
        controller = AuxPlotsController(self.image_layout)
        controller.add_from_type(AuxPlots.ProfilePlot)

        current_plots = set(controller.current_plots)
        layout_plots = set(self.image_layout.ci.items.keys())

        assert len(self.image_layout.ci.items) == 3
        assert current_plots.issubset(layout_plots)

        for plot in current_plots:
            assert not plot.isVisible()

        controller.show(True)

        for plot in current_plots:
            assert plot.isVisible()

        controller.clear()

        assert len(self.image_layout.ci.items) == 0
        assert controller.current_plots
