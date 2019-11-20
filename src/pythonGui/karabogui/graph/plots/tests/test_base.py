import os
from unittest.mock import patch

import numpy as np

from karabogui.testing import GuiTestCase

from ..base import KaraboPlotView

X_ARRAY = np.arange(10)
Y_ARRAY = X_ARRAY ** 2
FILENAME = "foo"
NPY = ".npy"
NPZ = ".npz"
NAMES = ["curve_1", "curve_2"]


class TestKaraboImageView(GuiTestCase):

    def setUp(self):
        super(TestKaraboImageView, self).setUp()
        self.widget = KaraboPlotView()

    def tearDown(self):
        super(TestKaraboImageView, self).tearDown()
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
