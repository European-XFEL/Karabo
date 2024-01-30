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

from karabogui.graph.common.api import AuxPlots, ROITool
from karabogui.testing import GuiTestCase

from ..base import KaraboImageView


class TestKaraboImageView(GuiTestCase):

    def setUp(self):
        super().setUp()
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
        assert not self.widget.plotItem.image_set
        assert self.aux_plots.current_plot == AuxPlots.NoPlot

        # Setup widget
        self.widget.show_aux_plots(AuxPlots.ProfilePlot)
        roi_item = self.roi.add(ROITool.Rect, pos=QPointF(3, 3), size=(3, 3))
        self.roi.show(ROITool.Rect)
        assert roi_item.isVisible()

        self._assert_aux_plots_data(has_data=False)

        # Set image
        image = np.ones((150, 100))
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        assert self.widget.plotItem.image_set
        assert np.array_equal(self.widget.plotItem.image, image)

        self._assert_aux_plots_data(has_data=True)

    def test_empty_np_image(self):
        """This test checks empty image with np.empty as input"""
        # Set image
        image = np.empty((100, 100))
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        assert self.widget.plotItem.image_set
        np.testing.assert_array_equal(self.widget.plotItem.image, image)

        assert self.aux_plots.current_plot == AuxPlots.NoPlot

    def test_empty_list_image(self):
        """This test checks empty image with [] as input"""
        # Set image
        image = np.array([])
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        assert not self.widget.plotItem.image_set

    def test_none_image(self):
        """This test checks empty image with None as input"""
        # Set image
        image = None
        self.widget.plotItem.set_image(image)

        # Check if image is properly set
        assert not self.widget.plotItem.image_set

    def _assert_aux_plots_data(self, has_data=True):
        current_plot = self.aux_plots.current_plot
        assert current_plot is not None

        assert_method = (self.assertFalse if has_data
                         else self.assertTrue)
        aggregator = self.aux_plots._aggregators[current_plot]
        for controller in aggregator.controllers.values():
            data_item = controller.plot._data_item
            assert_method(data_item.xData is None or len(data_item.xData) == 0)
            assert_method(data_item.xData is None or len(data_item.xData) == 0)

    def test_axes_labels(self):
        # Restore config
        self._assert_axes_labels(x_label='', x_units='',
                                 y_label='', y_units='')
        self._assert_axes_labels(x_label='Foo', x_units='',
                                 y_label='', y_units='')
        self._assert_axes_labels(x_label='', x_units='',
                                 y_label='Bar', y_units='')

    def _assert_axes_labels(self, **config):
        # Get axis items
        top = self.widget.plotItem.getAxis("top")
        bottom = self.widget.plotItem.getAxis("bottom")
        left = self.widget.plotItem.getAxis("left")
        right = self.widget.plotItem.getAxis("right")

        # Setup expected text and visibility
        x_label = f'{config["x_label"]}'
        if config["x_units"]:
            x_label += f' ({config["x_units"]})'
        x_visible = bool(config["x_label"] or config["x_units"])

        y_label = f'{config["y_label"]}'
        if config["y_units"]:
            y_label += f' ({config["y_units"]})'
        y_visible = bool(config["y_label"] or config["y_units"])

        # Apply config
        self.widget.restore(config)

        # Check blank axes
        assert top.labelText == x_label
        assert bottom.labelText == ''
        assert left.labelText == y_label
        assert right.labelText == ''

        # Check visibility
        assert top.label.isVisible() == x_visible
        assert bottom.label.isVisible() is False  # always false
        assert left.label.isVisible() == y_visible
        assert right.label.isVisible() is False  # always false
