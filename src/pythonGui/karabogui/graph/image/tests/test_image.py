# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import numpy as np
from qtpy.QtCore import QPointF

from karabogui.graph.common.api import (
    CrosshairROI, ImageROIController, RectROI, ROITool)
from karabogui.graph.image.plot import KaraboImagePlot
from karabogui.testing import GuiTestCase


class TestKaraboImagePlotROIController(GuiTestCase):

    def setUp(self):
        super().setUp()

        self.plot_item = KaraboImagePlot()
        self.roi_controller = ImageROIController(self.plot_item)
        self.roi_controller.enable_updates(True)
        self.roi_controller.updated.connect(self._mock_emit)

        self._region = None

        self.assertIsNone(self.roi_controller.current_tool_item)
        self.assertEqual(self.roi_controller.current_tool, ROITool.NoROI)

    def _mock_emit(self, value):
        self._region = value

    def test_image_rect_roi(self):
        roi = self.roi_controller.add(ROITool.Rect,
                                      pos=QPointF(-3, -3),
                                      size=(3, 3))

        image = np.zeros((125, 125))

        self.assertEqual(roi.center, QPointF(-1.5, -1.5))
        self.assertEqual(
            roi.textItem.toPlainText(),
            "Region of Interest\nCenter: (-1.5, -1.5)\nSize: (3, 3)")

        # 1. Check invalid ROI (out of bounds)
        self.plot_item.set_image(image)
        x_axis, y_axis = self.plot_item.transformed_axes
        self.assertFalse(self._region.valid())

        # 2. Check full intersection ROI
        roi.setPos(QPointF(15, 15))
        self.assertTrue(self._region.valid())
        x_slice, y_slice = self._region.slices
        self.assertTrue(np.array_equal(x_axis[x_slice], [15, 16, 17]))
        self.assertTrue(np.array_equal(y_axis[y_slice], [15, 16, 17]))

        # 3. Check partial intersection
        roi.setPos(QPointF(-1, -1))
        self.assertTrue(self._region.valid())
        x_slice, y_slice = self._region.slices
        self.assertTrue(np.array_equal(x_axis[x_slice], [0, 1]))
        self.assertTrue(np.array_equal(y_axis[y_slice], [0, 1]))

        self.assertIsInstance(self.roi_controller.current_tool_item, RectROI)
        self.assertEqual(self.roi_controller.current_tool, ROITool.Rect)

    def test_image_crosshair_roi(self):
        roi = self.roi_controller.add(ROITool.Crosshair,
                                      pos=QPointF(-3, -3))

        image = np.zeros((125, 125))

        # ROI is outside, so should emit invalid ImageRegion
        self.plot_item.set_image(image)
        self.assertFalse(self._region.valid())

        # Move the ROI to a valid x position (projects y-axis)
        roi.setPos(QPointF(-3, 3))
        self.assertEqual(roi.center, QPointF(-3.000000, 3.000000))
        self.assertEqual(roi.textItem.toPlainText(),
                         "Region of Interest\nCenter: (-3, 3)")

        self.plot_item.set_image(image)
        x_axis = self.plot_item.transformed_axes[0]
        x_slice = self._region.slices[0]

        self.assertFalse(self._region.valid(axis=1))
        self.assertTrue(self._region.valid(axis=0))
        self.assertTrue(np.array_equal(x_axis[x_slice], np.arange(125)))

        # Move the ROI to a valid y position (projects x-axis)
        roi.setPos(QPointF(3, -3))
        self.assertEqual(roi.center, QPointF(3.000000, -3.000000))
        self.assertEqual(roi.textItem.toPlainText(),
                         "Region of Interest\nCenter: (3, -3)")
        assert roi.coords == (3.000000, -3.000000)
        self.plot_item.set_image(image)
        y_axis = self.plot_item.transformed_axes[1]
        y_slice = self._region.slices[1]

        self.assertFalse(self._region.valid(axis=0))
        self.assertTrue(self._region.valid(axis=1))
        self.assertTrue(np.array_equal(y_axis[y_slice], np.arange(125)))

        self.assertIsInstance(self.roi_controller.current_tool_item,
                              CrosshairROI)
        self.assertEqual(self.roi_controller.current_tool, ROITool.Crosshair)
