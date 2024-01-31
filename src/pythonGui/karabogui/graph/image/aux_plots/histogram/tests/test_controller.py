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

from karabogui.graph.common.api import ImageRegion
from karabogui.testing import GuiTestCase

from ..controller import (
    HistogramAggregator, HistogramAnalyzer, HistogramController, HistogramPlot)


class TestHistogramAggregator(GuiTestCase):

    def setUp(self):
        super().setUp()
        self._aggregator = HistogramAggregator()
        self._aggregator.on_trait_change(self._mock_slot, "stats")

    def tearDown(self):
        super().tearDown()
        self._aggregator.on_trait_change(self._mock_slot, "stats", remove=True)
        self._emitted_value = None

    def _mock_slot(self, value):
        self._emitted_value = value

    def test_basics(self):
        assert len(self._aggregator.controllers) == 1
        aux_plot = self._aggregator.controller
        assert isinstance(aux_plot, HistogramController)
        assert isinstance(aux_plot.plot, HistogramPlot)
        assert isinstance(aux_plot.analyzer, HistogramAnalyzer)
        assert self._aggregator.show_stats

    def test_analyze(self):
        curve_item = self._aggregator.controller.plot._data_item
        analyzer = self._aggregator.controller.analyzer

        # 1. Check default
        np.testing.assert_array_equal(analyzer._hist, [])
        np.testing.assert_array_equal(analyzer._edges, [])
        np.testing.assert_array_equal(curve_item.xData, [])
        np.testing.assert_array_equal(curve_item.yData, [])

        # 2. Check valid region
        self._analyze_region(valid=True)
        assert analyzer._hist is not None
        assert analyzer._edges is not None
        np.testing.assert_array_equal(curve_item.xData, analyzer._edges)
        np.testing.assert_array_equal(curve_item.yData, analyzer._hist)

        # 3. Check invalid region
        self._analyze_region(valid=False)
        np.testing.assert_array_equal(analyzer._hist, [])
        np.testing.assert_array_equal(analyzer._edges, [])
        np.testing.assert_array_equal(curve_item.xData, [])
        np.testing.assert_array_equal(curve_item.yData, [])

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

        assert region.valid() is valid
        self._aggregator.process(region)

    def _assert_show_stats(self, valid=True):
        assertion = self.assertNotEqual if valid else self.assertEqual

        # Start from disabled stats
        if self._aggregator.show_stats:
            self._aggregator.show_stats = False
            assert self._emitted_value is None

        # Show stats
        self._aggregator.show_stats = True
        assertion(self._emitted_value.html, '')

        # Hide stats
        self._aggregator.show_stats = False
        assert self._emitted_value is None
