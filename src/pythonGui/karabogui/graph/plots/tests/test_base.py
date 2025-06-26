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
import os
from unittest.mock import patch

import numpy as np
from qtpy.QtCore import QPointF

from karabo.common.scenemodel.api import CurveType
from karabogui.graph.common.api import CrosshairROI, ROITool, safe_log10
from karabogui.testing import GuiTestCase

from ..base import KaraboPlotView

X_ARRAY = np.arange(10)
Y_ARRAY = X_ARRAY ** 2

FILENAME = "foo"
NPY = ".npy"
NPZ = ".npz"
NAMES = ["curve_1", "curve_2"]


class TestPlotViewExport(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.widget = KaraboPlotView()

    def tearDown(self):
        super().tearDown()
        self.widget.destroy()
        self.widget = None
        for ext in [NPY, NPZ]:
            self._delete_file(FILENAME + ext)

    def test_export_curve_item(self):
        # Test empty plot widget
        self._assert_npy_export(exported=False)

        # Test curves with no data
        curve_1 = self.widget.add_curve_item(name=NAMES[0])
        self._assert_npy_export(has_data=False)

        # Test curves with data
        curve_1.setData(X_ARRAY, Y_ARRAY)
        self._assert_npy_export()

        # # Test multiple curves
        curve_2 = self.widget.add_curve_item(name=NAMES[1])
        self._assert_npz_export(names=NAMES, has_data=[True, False])

        # # Test multiple curves
        curve_2.setData(X_ARRAY, Y_ARRAY)
        self._assert_npz_export(names=NAMES, has_data=[True, True])

    def test_export_histogram(self):
        curve = self.widget.add_curve_item(name=NAMES[0])

        # Test curves with data
        curve.setData(np.concatenate((X_ARRAY, [10])), Y_ARRAY,
                      stepMode="center")
        self._assert_npy_export()

    def test_export_scatter_item(self):
        # Test curve fill with no data
        item = self.widget.add_scatter_item()
        self._assert_npy_export(has_data=False)

        item.setData(X_ARRAY, Y_ARRAY)
        self._assert_npy_export()

    def _assert_npy_export(self, exported=True, has_data=True):
        filename = FILENAME + NPY
        self._assert_exported(filename, exported)
        if exported:
            self._assert_ndarray(np.load(filename), has_data)

    def _assert_npz_export(self, exported=True, names=(None, None),
                           has_data=(True, True)):
        filename = FILENAME + NPZ
        self._assert_exported(filename, exported)

        if exported:
            zipped = np.load(filename)
            assert isinstance(zipped, np.lib.npyio.NpzFile)
            assert set(names) == set(zipped.files)
            for index, name in enumerate(names):
                self._assert_ndarray(zipped[name], has_data[index])

    def _assert_exported(self, filename, exported):
        self._delete_file(filename)
        assert not os.path.isfile(filename)

        with patch('karabogui.util.getSaveFileName', return_value=filename):
            self.widget.export()

        assert os.path.isfile(filename) is exported

    def _assert_ndarray(self, data, has_data=True):
        x_array = X_ARRAY if has_data else []
        y_array = Y_ARRAY if has_data else []
        assert isinstance(data, np.ndarray)
        np.testing.assert_array_equal(data[0], x_array)
        np.testing.assert_array_equal(data[1], y_array)

    def _delete_file(self, filename):
        if os.path.isfile(filename):
            os.remove(filename)


DEFAULT_CONFIG = {"x_grid": False, "y_grid": False,
                  "x_log": False, "y_log": False,
                  "x_invert": False, "y_invert": False,
                  "x_label": '', "y_label": '',
                  "x_units": '', "y_units": '',
                  "x_autorange": True, "y_autorange": True,
                  "roi_items": [], "roi_tool": 0,
                  "background": "transparent", "title": ""}

orders = np.arange(-5, 5, dtype=np.float64)
LOG_ARRAY = 10 ** orders
RANGE_TOLERANCE = 0.12  # or 12% percent


class _BasePlotTest(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.widget = KaraboPlotView()
        self.widget.configuration.update(DEFAULT_CONFIG)
        self.widget.show()
        self.viewbox = self.widget.plotItem.vb
        self._plot = None

    def tearDown(self):
        super().tearDown()
        self.widget.close()
        self.widget.destroy()
        self.widget = None

    def restore(self, **kwargs):
        config = DEFAULT_CONFIG.copy()
        config.update(kwargs)
        self.widget.restore(config)

    def set_data(self, x, y):
        self._plot.setData(x, y)
        # Trigger auto range
        self.viewbox.updateAutoRange()

    def set_log_scale(self, x=None, y=None):
        if x is not None:
            self.widget.set_log_x(x)
            self.widget.configuration['x_log'] = x
        if y is not None:
            self.widget.set_log_y(y)
            self.widget.configuration['y_log'] = y
        self.viewbox.updateAutoRange()

    def set_range_x(self, low, high):
        expected = {"x_min": low, "x_max": high, "x_autorange": False}
        with patch("karabogui.graph.plots.base.RangeDialog.get",
                   return_value=(expected, True)):
            self.widget.configure_range_x()

    def set_range_y(self, low, high):
        expected = {"y_min": low, "y_max": high, "y_autorange": False}
        with patch("karabogui.graph.plots.base.RangeDialog.get",
                   return_value=(expected, True)):
            self.widget.configure_range_y()

    def assert_data(self, x_expected, y_expected, log_x=False, log_y=False):
        # Check getData values
        x_data, y_data = self._plot.getData()
        if log_x:
            x_expected = safe_log10(x_expected)
            np.testing.assert_almost_equal(x_data, x_expected)
        else:
            np.testing.assert_array_equal(x_data, x_expected)
        if log_y:
            y_expected = safe_log10(y_expected)
            np.testing.assert_almost_equal(y_data, y_expected)
        else:
            np.testing.assert_array_equal(y_data, y_expected)

    def assert_range(self, x_expected, y_expected, log_x=False, log_y=False):
        self.assert_range_x(x_expected, log_x)
        self.assert_range_y(y_expected, log_y)

    def assert_range_x(self, x_expected, log_x):
        assert_x_range = (self.assert_log_range if log_x
                          else self.assert_linear_range)
        assert_x_range(actual=self.widget.get_view_range_x(),
                       expected=x_expected)

    def assert_range_y(self, y_expected, log_y):
        assert_y_range = (self.assert_log_range if log_y
                          else self.assert_linear_range)
        assert_y_range(actual=self.widget.get_view_range_y(),
                       expected=y_expected)

    def assert_log_range(self, actual, expected):
        """The tolerance is by its order with the deviation of
           12% the of range """
        actual = np.ceil(np.log10(actual))
        # Calculate expected range
        log_expected = safe_log10(expected)
        expected = np.nanmin(log_expected), np.nanmax(log_expected)
        # Calculate tolerance, expect a range of at least 1 order.
        tol = int((expected[-1] - expected[0]) * 0.12) or 1
        np.testing.assert_allclose(actual, expected, atol=tol)

    def assert_linear_range(self, actual, expected):
        tol = (expected[-1] - expected[0]) * RANGE_TOLERANCE
        np.testing.assert_allclose(actual, [expected[0], expected[-1]],
                                   atol=tol)


class TestCurveItem(_BasePlotTest):

    def setUp(self):
        super().setUp()
        self._plot = self.widget.add_curve_item()

    def test_basics(self):
        self.set_data(x=X_ARRAY, y=LOG_ARRAY)
        self.assert_data(x_expected=X_ARRAY, y_expected=LOG_ARRAY)
        self.assert_range(x_expected=X_ARRAY, y_expected=LOG_ARRAY)

    def test_log_scale(self):
        self.set_data(x=X_ARRAY, y=LOG_ARRAY)
        self.set_log_scale(y=True)
        self.assert_data(x_expected=X_ARRAY, y_expected=LOG_ARRAY, log_y=True)
        # Check if the range doesn't change with log y scale
        self.assert_range(x_expected=X_ARRAY, y_expected=LOG_ARRAY,
                          log_y=True)

    def test_ranges(self):
        self.set_data(x=X_ARRAY, y=LOG_ARRAY)
        self.set_log_scale(y=True)

        # Set range
        low, high = 1e-9, 1e9
        self.set_range_y(low, high)
        self.assert_range_y(y_expected=(low, high), log_y=True)

        # Set problematic range (with zeros)
        low, high = 0, 1e4
        self.set_range_y(low, high)
        corrected_low = 10 ** safe_log10(low)  # 1e-25
        self.assert_range_y(y_expected=(corrected_low, high), log_y=True)

    def test_legend_text(self):
        legend = self.widget.add_legend()
        plot = self.widget.add_curve_item(name="test_plot")
        label = legend.getLabel(plot)
        assert label.text == "test_plot"

        self.widget.update_legend_text(plot, "new_legend")
        label = legend.getLabel(plot)
        assert label.text == "new_legend"

    def test_apply_options(self):
        plot_1_color = self._plot.opts["pen"].color().name()
        legend = self.widget.add_legend()
        plot = self.widget.add_curve_item(name="test_plot")
        options = {plot: {"pen_color": "#ff7f00",
                          "curve_type": CurveType.Curve,
                          "name": "new_legend"}}
        self.widget.apply_curve_options(options)
        label = legend.getLabel(plot)
        assert label.text == "new_legend"
        plot.opts["pen"].color().name() == "#ff7f00"

        # When data points are shown, very they are updated too.
        self.widget._show_symbols = True
        self.widget.apply_curve_options(options)
        assert plot.opts["symbolPen"].color().name() == "#ff7f00"
        assert plot.opts["symbolBrush"].color().name() == "#ff7f00"

        # The other curve should have no change.
        assert legend.getLabel(self._plot) is None
        assert self._plot.opts["pen"].color().name() == plot_1_color


class TestBarItem(_BasePlotTest):

    def setUp(self):
        super().setUp()
        self._plot = self.widget.add_bar_item()
        self._plot.set_width(0.1)

    def test_basics(self):
        self.set_data(x=X_ARRAY, y=LOG_ARRAY)
        self._assert_bar(x_expected=X_ARRAY, y_expected=LOG_ARRAY)
        self.assert_data(x_expected=X_ARRAY, y_expected=LOG_ARRAY)
        self.assert_range(x_expected=X_ARRAY, y_expected=LOG_ARRAY)

    def test_log_scale(self):
        self.set_data(x=X_ARRAY, y=LOG_ARRAY)
        self.set_log_scale(y=True)
        self._assert_bar(x_expected=X_ARRAY, y_expected=LOG_ARRAY)
        self.assert_data(x_expected=X_ARRAY, y_expected=LOG_ARRAY, log_y=True)
        # Check if the range doesn't change with log y scale
        self.assert_range(x_expected=X_ARRAY, y_expected=LOG_ARRAY,
                          log_y=True)

    def test_problematic_log_scale(self):
        y_array = np.array([0, 0.001, 0, -0.001])
        x_array = np.arange(y_array.size)
        self.set_data(x=x_array, y=y_array)
        self.set_log_scale(y=True)
        self._assert_bar(x_expected=x_array, y_expected=y_array)
        self.assert_data(x_expected=x_array, y_expected=y_array, log_y=True)
        # Check if the range doesn't change with log y scale
        self.assert_range(x_expected=x_array, y_expected=y_array,
                          log_y=True)

    def _assert_bar(self, x_expected, y_expected):
        # Check bar attributes
        np.testing.assert_array_equal(self._plot.opts.get('x'),
                                      x_expected)
        np.testing.assert_array_equal(self._plot.opts.get('height'),
                                      y_expected)


class TestPlotViewRestore(_BasePlotTest):

    def test_restore_roi(self):
        controller = self.widget.add_roi()
        toolbar = self.widget.add_toolbar()

        # Restore ROI items
        roi_tool = int(ROITool.Crosshair)
        roi_items = [{"roi_type": roi_tool, "x": 15, "y": 15, "name": "ROI"}]
        self.restore(roi_items=roi_items, roi_tool=roi_tool)

        # Check if only one ROI type is present and current
        assert controller.current_tool == roi_tool
        assert len(controller.roi_items) == 1

        # Check if only one ROI item is present
        crosshairs = controller.roi_items[roi_tool]
        assert len(crosshairs) == 1

        # Check ROI item and coords
        crosshair = crosshairs[0]
        assert isinstance(crosshair, CrosshairROI)
        x, y = crosshair.coords
        assert x == 15
        assert y == 15
        assert crosshair.center == QPointF(15.000000, 15.000000)

        # Check if tool button is checked
        assert toolbar.buttons[ROITool(roi_tool).name].isChecked()

    def test_restore_manual_ranges(self):
        # Set range with with log
        low, high = 1e-9, 1e9
        self.restore(y_min=low, y_max=high, y_autorange=False, y_log=True)
        self.assert_range_y(y_expected=(low, high), log_y=True)

        # Set some data and check if range will not change
        self._plot = self.widget.add_curve_item()
        self.set_data(x=X_ARRAY, y=LOG_ARRAY)
        self.assert_range_y(y_expected=(low, high), log_y=True)

    def test_axis_visibility(self):
        bottom_axes = self.widget.plotItem.getAxis("bottom")
        left_axes = self.widget.plotItem.getAxis("left")
        x_label = bottom_axes.label
        y_label = left_axes.label
        assert x_label.isVisible() is False
        assert y_label.isVisible() is False

        self.restore(x_label="Axis", y_label="Axis")
        assert x_label.isVisible() is True
        assert y_label.isVisible() is True

        self.restore(x_label="", y_label="")
        assert x_label.isVisible() is False
        assert y_label.isVisible() is False

        self.restore(x_units="mm", y_units="mm")
        assert x_label.isVisible() is True
        assert y_label.isVisible() is True

        self.restore(x_units="", y_units="")
        assert x_label.isVisible() is False
        assert y_label.isVisible() is False

    def test_title_background(self):
        toolbar = self.widget.add_toolbar()
        self.restore(background="white")
        brush = self.widget.graph_view.backgroundBrush()
        assert brush.color().getRgb() == (255, 255, 255, 255)
        self.restore(background="red")
        brush = self.widget.graph_view.backgroundBrush()
        assert brush.color().getRgb() == (255, 0, 0, 255)
        assert self.widget._toolbar is not None

        tb = toolbar.widget
        sheet = tb.styleSheet()
        assert "background-color: rgba(255, 0, 0, 255)" in sheet
