from PyQt4.QtCore import QPointF
from karabogui.graph.common.api import ImageROIController, ROITool
from karabogui.graph.image.plot import KaraboImagePlot
from karabogui.testing import GuiTestCase

import numpy as np


class TestKaraboImagePlotROIController(GuiTestCase):

    def setUp(self):
        super().setUp()

        self.plot_item = KaraboImagePlot()
        self.roi_controller = ImageROIController(self.plot_item)
        self.roi_controller.enable_updates(True)
        self.roi_controller.updated.connect(self._mock_emit)

        self._region = None

    def _mock_emit(self, value):
        self._region = value

    def test_image_rect_roi(self):
        roi = self.roi_controller.add(ROITool.Rect,
                                      pos=QPointF(-3, -3),
                                      size=(3, 3))

        image = np.zeros((125, 125))

        # 1. Check invalid ROI (out of bounds)
        self.plot_item.set_image(image)
        self.assertFalse(self._region.valid())

        # 2. Check full intersection ROI
        roi.setPos(QPointF(15, 15))
        self.assertTrue(self._region.valid())
        x_axis, y_axis = self._region.axes
        self.assertTrue(np.array_equal(x_axis, [15, 16, 17]))
        self.assertTrue(np.array_equal(y_axis, [15, 16, 17]))

        # 3. Check partial intersection
        roi.setPos(QPointF(-1, -1))
        self.assertTrue(self._region.valid())
        x_axis, y_axis = self._region.axes
        self.assertTrue(np.array_equal(x_axis, [0, 1]))
        self.assertTrue(np.array_equal(y_axis, [0, 1]))

    def test_image_crosshair_roi(self):
        roi = self.roi_controller.add(ROITool.Crosshair,
                                      pos=QPointF(-3, -3))

        image = np.zeros((125, 125))

        # ROI is outside, so should emit invalid ImageRegion
        self.plot_item.set_image(image)
        self.assertFalse(self._region.valid())

        # Move the ROI to a valid x position (projects y-axis)
        roi.setPos(QPointF(-3, 3))
        self.plot_item.set_image(image)
        self.assertFalse(self._region.valid(axis=1))
        self.assertTrue(self._region.valid(axis=0))
        self.assertTrue(np.array_equal(self._region.axes[0], np.arange(125)))

        # Move the ROI to a valid y position (projects x-axis)
        roi.setPos(QPointF(3, -3))
        self.plot_item.set_image(image)
        self.assertFalse(self._region.valid(axis=0))
        self.assertTrue(self._region.valid(axis=1))
        self.assertTrue(np.array_equal(self._region.axes[1], np.arange(125)))
