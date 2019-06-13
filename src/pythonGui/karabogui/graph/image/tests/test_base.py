import numpy as np
from PyQt4.QtCore import QPointF

from karabogui.testing import GuiTestCase

from karabogui.graph.common.enums import AuxPlots, ROITool
from ..base import KaraboImageView


class TestKaraboImageView(GuiTestCase):

    def setUp(self):
        super(TestKaraboImageView, self).setUp()
        self.widget = KaraboImageView()
        self.roi = self.widget.add_roi()
        self.aux_plots = self.widget.add_aux(AuxPlots.ProfilePlot)

    def tearDown(self):
        self.roi.destroy()
        self.aux_plots.destroy()
        self.widget.destroy()

    def test_first_image(self):
        """ This test checks when the image is set,
            the aux plots should have data """

        # Check no data on init
        self.assertFalse(self.widget.plotItem.image_set)
        self._assert_aux_plots_data(has_data=False)

        # Setup widget
        self.widget.show_aux_plots(AuxPlots.ProfilePlot)
        roi_item = self.roi.add(ROITool.Rect, pos=QPointF(3, 3), size=(3, 3))
        self.roi.show(ROITool.Rect)
        self.assertTrue(roi_item.isVisible())

        self._assert_aux_plots_data(has_data=False)

        # Set image
        image = np.ones((150, 100))
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        self.assertTrue(self.widget.plotItem.image_set)
        self.assertTrue(np.array_equal(self.widget.plotItem.image, image))

        self._assert_aux_plots_data(has_data=True)

    def _assert_aux_plots_data(self, has_data=True):
        assert_method = self.assertIsNotNone if has_data else self.assertIsNone

        for ax in range(2):
            data_item = self.aux_plots.current_plots[ax]._line
            assert_method(data_item.xData)
            assert_method(data_item.yData)
