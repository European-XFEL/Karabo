import numpy as np

from karabogui.testing import GuiTestCase
from karabogui.graph.common.api import ImageRegion

from ..controller import IntensityProfiler, ProfilePlotController, StepPlot

X_LENGTH, Y_LENGTH = (5, 4)


class TestProfilingController(GuiTestCase):

    def setUp(self):
        super(TestProfilingController, self).setUp()
        self._controller = ProfilePlotController()
        self._controller.set_axes(x_data=np.arange(X_LENGTH),
                                  y_data=np.arange(Y_LENGTH))
        self._controller.showStatsRequested.connect(self._mock_slot)

    def tearDown(self):
        super(TestProfilingController, self).tearDown()
        self._controller.disconnect()
        self._controller.deleteLater()
        self._emitted_value = None

    def _mock_slot(self, value):
        self._emitted_value = value

    def test_basics(self):
        self.assertEqual(len(self._controller.plots), 2)
        for plot in self._controller.plots:
            self.assertIsInstance(plot, StepPlot)

        self.assertEqual(len(self._controller.plots), 2)
        for profiler in self._controller.analyzers:
            self.assertIsInstance(profiler, IntensityProfiler)

        self.assertFalse(self._controller._fitted)
        self.assertFalse(self._controller._stats_enabled)

    def test_analyze(self):
        # 1. Check default
        self._assert_analyze(valid=False, fitted=False)

        # 2. Check valid region
        self._analyze_region(valid=True)
        self._assert_analyze(valid=True, fitted=False)

        self._analyze_region(valid=True, fitted=True)
        self._assert_analyze(valid=True, fitted=True)

        # 3. Check invalid region
        self._analyze_region(valid=False)
        self._assert_analyze(valid=False, fitted=False)

        self._analyze_region(valid=False, fitted=True)
        self._assert_analyze(valid=False, fitted=False)

    def _analyze_region(self, valid=True, fitted=False):
        region = ImageRegion()
        if valid:
            image = np.arange(X_LENGTH * Y_LENGTH).reshape(Y_LENGTH, X_LENGTH)
            region = ImageRegion(region=image,
                                 region_type=ImageRegion.Area,
                                 x_slice=slice(X_LENGTH),
                                 y_slice=slice(Y_LENGTH))

        self.assertTrue(region.valid() is valid)
        self._controller._stats_enabled = self._controller._fitted = fitted
        self._controller.analyze(region)

    def _assert_analyze(self, valid=True, fitted=True):
        for plot, analyzer in self._controller._plots:
            data_curve, fit_curve = plot._data_items

            # Check profile values
            if valid:
                data_assertion = self.assertIsNotNone
                x_data, y_data = analyzer._data
                # Add one more vestigial data at the end because of step plot
                x_data = np.hstack((x_data, [x_data[-1] + 1]))
            else:
                data_assertion = self.assertIsNone
                x_data, y_data = [], []

            data_assertion(analyzer._data)
            np.testing.assert_array_equal(data_curve.xData, x_data)
            np.testing.assert_array_equal(data_curve.yData, y_data)

            # Check fit values
            if fitted:
                fit_assertion = self.assertIsNotNone
                x_fit, y_fit = analyzer._fit
            else:
                fit_assertion = self.assertIsNone
                x_fit, y_fit = [], []

            fit_assertion(analyzer._fit)
            np.testing.assert_array_equal(fit_curve.xData, x_fit)
            np.testing.assert_array_equal(fit_curve.yData, y_fit)

    def test_get_html(self):
        # 1. Check default
        stats = self._controller.get_html()
        self.assertEqual(stats, '')

        # 2. Check valid region, not fitted
        self._analyze_region(valid=True, fitted=False)
        stats = self._controller.get_html()
        self.assertEqual(stats, '')

        # 2. Check valid region, fitted
        self._analyze_region(valid=True, fitted=True)
        stats = self._controller.get_html()
        self.assertNotEqual(stats, '')

        # 3. Check invalid region
        self._analyze_region(valid=False, fitted=False)
        stats = self._controller.get_html()
        self.assertEqual(stats, '')

        # 4. Check invalid region, fitted
        self._analyze_region(valid=False, fitted=True)
        stats = self._controller.get_html()
        self.assertEqual(stats, '')

    def test_enable_fitting(self):
        # 1. Check default
        self._assert_show_stats(valid=False)

        # 2. Check valid region
        self._analyze_region(valid=True)
        self._assert_show_stats(valid=True)

        # 3. Check invalid region
        self._analyze_region(valid=False)
        self._assert_show_stats(valid=False)

    def _assert_show_stats(self, valid=True):
        assertion = self.assertNotEqual if valid else self.assertEqual

        # Start from disabled stats
        if self._controller._stats_enabled:
            self._controller._enable_fitting(False)
            self.assertFalse(self._controller._stats_enabled)

        # Show stats
        self._controller._enable_fitting(True)
        self.assertTrue(self._controller._stats_enabled)
        assertion(self._emitted_value, '')

        # Hide stats
        self._controller._enable_fitting(False)
        self.assertFalse(self._controller._stats_enabled)
        self.assertEqual(self._emitted_value, '')
