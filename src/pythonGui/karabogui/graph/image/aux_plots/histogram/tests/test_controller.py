import numpy as np

from karabogui.testing import GuiTestCase
from karabogui.graph.common.api import ImageRegion

from ..controller import HistogramController, HistogramPlot, Histogram


class TestHistogramController(GuiTestCase):

    def setUp(self):
        super(TestHistogramController, self).setUp()
        self._controller = HistogramController()
        self._controller.showStatsRequested.connect(self._mock_slot)
        self._plot = self._controller.plots[0]

    def tearDown(self):
        super(TestHistogramController, self).tearDown()
        self._controller.disconnect()
        self._controller.deleteLater()
        self._emitted_value = None

    def _mock_slot(self, value):
        self._emitted_value = value

    def test_basics(self):
        self.assertEqual(len(self._controller.plots), 1)
        plot, analyzer = self._controller._plots[0]
        self.assertIsInstance(plot, HistogramPlot)
        self.assertIsInstance(analyzer, Histogram)

    def test_analyze(self):
        curve_item = self._plot._data_items[0]
        analyzer = self._controller.analyzers[0]

        # 1. Check default
        self.assertIsNone(analyzer._hist)
        self.assertIsNone(analyzer._edges)
        np.testing.assert_array_equal(curve_item.xData, [])
        np.testing.assert_array_equal(curve_item.yData, [])

        # 2. Check valid region
        self._analyze_region(valid=True)
        self.assertIsNotNone(analyzer._hist)
        self.assertIsNotNone(analyzer._edges)
        np.testing.assert_array_equal(curve_item.xData, analyzer._edges)
        np.testing.assert_array_equal(curve_item.yData, analyzer._hist)

        # 3. Check invalid region
        self._analyze_region(valid=False)
        self.assertIsNone(analyzer._hist)
        self.assertIsNone(analyzer._edges)
        np.testing.assert_array_equal(curve_item.xData, [])
        np.testing.assert_array_equal(curve_item.yData, [])

    def test_get_html(self):
        # 1. Check default
        stats = self._controller.get_html()
        self.assertEqual(stats, '')

        # 2. Check valid region
        self._analyze_region(valid=True)
        stats = self._controller.get_html()
        self.assertNotEqual(stats, '')

        # 3. Check invalid region
        self._analyze_region(valid=False)
        stats = self._controller.get_html()
        self.assertEqual(stats, '')

    def test_show_stats(self):
        # 1. Check default
        self._assert_show_stats(valid=False)

        # 2. Check valid region
        self._analyze_region(valid=True)
        self._assert_show_stats(valid=True)

        # 3. Check invalid region
        self._analyze_region(valid=False)
        self._assert_show_stats(valid=False)

    def _analyze_region(self, valid=True):
        region = ImageRegion()
        if valid:
            region = ImageRegion(region=np.arange(20).reshape(4, 5),
                                 region_type=ImageRegion.Area,
                                 x_slice=slice(5),
                                 y_slice=slice(4))

        self.assertTrue(region.valid() is valid)
        self._controller.analyze(region)

    def _assert_show_stats(self, valid=True):
        assertion = self.assertNotEqual if valid else self.assertEqual

        # Start from disabled stats
        if self._controller._stats_enabled:
            self._plot.show_stats_action.trigger()
            self.assertFalse(self._controller._stats_enabled)

        # Show stats
        self._plot.show_stats_action.trigger()
        self.assertTrue(self._controller._stats_enabled)
        assertion(self._emitted_value, '')

        # Hide stats
        self._plot.show_stats_action.trigger()
        self.assertFalse(self._controller._stats_enabled)
        self.assertEqual(self._emitted_value, '')
