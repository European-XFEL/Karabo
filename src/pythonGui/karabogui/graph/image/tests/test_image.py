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

        assert self.roi_controller.current_tool_item is None
        assert self.roi_controller.current_tool == ROITool.NoROI

    def _mock_emit(self, value):
        self._region = value

    def test_image_rect_roi(self):
        roi = self.roi_controller.add(ROITool.Rect,
                                      pos=QPointF(-3, -3),
                                      size=(3, 3))

        image = np.zeros((125, 125))

        assert roi.center == QPointF(-1.5, -1.5)
        assert roi.textItem.toPlainText() == (
            "Region of Interest\nCenter: (-1.5, -1.5)\nSize: (3, 3)")

        # 1. Check invalid ROI (out of bounds)
        self.plot_item.set_image(image)
        x_axis, y_axis = self.plot_item.transformed_axes
        assert not self._region.valid()

        # 2. Check full intersection ROI
        roi.setPos(QPointF(15, 15))
        assert self._region.valid()
        x_slice, y_slice = self._region.slices
        assert np.array_equal(x_axis[x_slice], [15, 16, 17])
        assert np.array_equal(y_axis[y_slice], [15, 16, 17])

        # 3. Check partial intersection
        roi.setPos(QPointF(-1, -1))
        assert self._region.valid()
        x_slice, y_slice = self._region.slices
        assert np.array_equal(x_axis[x_slice], [0, 1])
        assert np.array_equal(y_axis[y_slice], [0, 1])

        assert isinstance(self.roi_controller.current_tool_item, RectROI)
        assert self.roi_controller.current_tool == ROITool.Rect

    def test_image_crosshair_roi(self):
        roi = self.roi_controller.add(ROITool.Crosshair,
                                      pos=QPointF(-3, -3))

        image = np.zeros((125, 125))

        # ROI is outside, so should emit invalid ImageRegion
        self.plot_item.set_image(image)
        assert not self._region.valid()

        # Move the ROI to a valid x position (projects y-axis)
        roi.setPos(QPointF(-3, 3))
        assert roi.center == QPointF(-3.000000, 3.000000)
        assert roi.textItem.toPlainText() == \
               "Region of Interest\nCenter: (-3, 3)"

        self.plot_item.set_image(image)
        x_axis = self.plot_item.transformed_axes[0]
        x_slice = self._region.slices[0]

        assert not self._region.valid(axis=1)
        assert self._region.valid(axis=0)
        assert np.array_equal(x_axis[x_slice], np.arange(125))

        # Move the ROI to a valid y position (projects x-axis)
        roi.setPos(QPointF(3, -3))
        assert roi.center == QPointF(3.000000, -3.000000)
        assert roi.textItem.toPlainText() == (
            "Region of Interest\nCenter: (3, -3)")
        assert roi.coords == (3.000000, -3.000000)
        self.plot_item.set_image(image)
        y_axis = self.plot_item.transformed_axes[1]
        y_slice = self._region.slices[1]

        assert not self._region.valid(axis=0)
        assert self._region.valid(axis=1)
        assert np.array_equal(y_axis[y_slice], np.arange(125))

        assert isinstance(self.roi_controller.current_tool_item, CrosshairROI)
        assert self.roi_controller.current_tool == ROITool.Crosshair
