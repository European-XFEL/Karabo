import os
from unittest.mock import patch

import numpy as np
from numpy.testing import assert_allclose, assert_array_equal

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
        super(TestPlotViewExport, self).setUp()
        self.widget = KaraboPlotView()

    def tearDown(self):
        super(TestPlotViewExport, self).tearDown()
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
        curve.setData(np.concatenate((X_ARRAY, [10])), Y_ARRAY, stepMode=True)
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
            self.assertIsInstance(zipped, np.lib.npyio.NpzFile)
            self.assertSetEqual(set(names), set(zipped.files))
            for index, name in enumerate(names):
                self._assert_ndarray(zipped[name], has_data[index])

    def _assert_exported(self, filename, exported):
        self._delete_file(filename)
        self.assertFalse(os.path.isfile(filename))

        with patch('karabogui.util.getSaveFileName', return_value=filename):
            self.widget.export()

        self.assertTrue(os.path.isfile(filename) is exported)

    def _assert_ndarray(self, data, has_data=True):
        x_array = X_ARRAY if has_data else []
        y_array = Y_ARRAY if has_data else []
        self.assertIsInstance(data, np.ndarray)
        np.testing.assert_array_equal(data[0], x_array)
        np.testing.assert_array_equal(data[1], y_array)

    def _delete_file(self, filename):
        if os.path.isfile(filename):
            os.remove(filename)


orders = np.arange(-5, 5, dtype=np.float)
LOG_ARRAY = 10 ** orders
RANGE_TOLERANCE = 0.12  # or 12% percent


class _BasePlotTest(GuiTestCase):

    def setUp(self):
        super(_BasePlotTest, self).setUp()
        self.widget = KaraboPlotView()
        self.widget.configuration.update({'x_log': False, 'y_log': False})
        self.widget.show()
        self.viewbox = self.widget.plotItem.vb
        self._plot = None

    def tearDown(self):
        super(_BasePlotTest, self).tearDown()
        self.widget.close()
        self.widget.destroy()
        self.widget = None

    def _set_data(self, x, y):
        self._plot.setData(x, y)
        # Trigger auto range
        self.viewbox.updateAutoRange()

    def _set_log_scale(self, x=None, y=None):
        if x is not None:
            self.widget.set_log_x(x)
            self.widget.configuration['x_log'] = x
        if y is not None:
            self.widget.set_log_y(y)
            self.widget.configuration['y_log'] = y
        self.viewbox.updateAutoRange()

    def _assert_data(self, x_expected, y_expected, log_x=False, log_y=False):
        # Check getData values
        x_data, y_data = self._plot.getData()
        if log_x:
            x_expected = np.log10(x_expected)
        if log_y:
            y_expected = np.log10(y_expected)
        assert_array_equal(x_data, x_expected)
        assert_array_equal(y_data, y_expected)

    def _assert_range(self, x_expected, y_expected, log_x=False, log_y=False):
        assert_x_range = (self._assert_log_range if log_x
                          else self._assert_linear_range)
        assert_y_range = (self._assert_log_range if log_y
                          else self._assert_linear_range)

        assert_x_range(actual=self.widget.get_view_range_x(),
                       expected=x_expected)
        assert_y_range(actual=self.widget.get_view_range_y(),
                       expected=y_expected)

    def _assert_log_range(self, actual, expected):
        """The tolerance is by its order with the deviation of +/-1 """
        actual = np.ceil(np.log10(actual))
        expected = np.log10([expected[0], expected[-1]])
        assert_allclose(actual, expected, atol=1)

    def _assert_linear_range(self, actual, expected):
        tol = (expected[-1] - expected[0]) * RANGE_TOLERANCE
        assert_allclose(actual, [expected[0], expected[-1]], atol=tol)


class TestCurveItem(_BasePlotTest):

    def setUp(self):
        super(TestCurveItem, self).setUp()
        self._plot = self.widget.add_curve_item()

    def test_basics(self):
        self._set_data(x=X_ARRAY, y=LOG_ARRAY)
        self._assert_data(x_expected=X_ARRAY, y_expected=LOG_ARRAY)
        self._assert_range(x_expected=X_ARRAY, y_expected=LOG_ARRAY)
