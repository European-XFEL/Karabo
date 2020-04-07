import numpy as np
from PyQt5.QtCore import QPointF

from karabogui.testing import GuiTestCase

from karabogui.graph.common.api import AuxPlots, ROITool
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
        self.widget = None

    def test_first_image(self):
        """ This test checks when the image is set,
            the aux plots should have data """

        # Check no data on init
        self.assertFalse(self.widget.plotItem.image_set)
        self.assertEqual(self.aux_plots.current_plot, AuxPlots.NoPlot)

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

    def test_empty_np_image(self):
        """This test checks empty image with np.empty as input"""
        # Set image
        image = np.empty((100, 100))
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        self.assertTrue(self.widget.plotItem.image_set)
        np.testing.assert_array_equal(self.widget.plotItem.image, image)

        self.assertEqual(self.aux_plots.current_plot, AuxPlots.NoPlot)

    def test_empty_list_image(self):
        """This test checks empty image with [] as input"""
        # Set image
        image = np.array([])
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        self.assertFalse(self.widget.plotItem.image_set)

    def test_none_image(self):
        """This test checks empty image with None as input"""
        # Set image
        image = None
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        self.assertFalse(self.widget.plotItem.image_set)

    def _assert_aux_plots_data(self, has_data=True):
        current_plot = self.aux_plots.current_plot
        self.assertIsNotNone(current_plot)

        assert_method = self.assertTrue if has_data else self.assertFalse
        aggregator = self.aux_plots._aggregators[current_plot]
        for controller in aggregator.controllers.values():
            data_item = controller.plot._data_item
            assert_method(len(data_item.xData) != 0)
            assert_method(len(data_item.yData) != 0)
