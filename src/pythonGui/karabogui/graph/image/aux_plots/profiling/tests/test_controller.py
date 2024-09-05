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
import pytest

from karabogui.graph.common.api import ImageRegion
from karabogui.testing import GuiTestCase

from ..controller import (
    ProfileAggregator, ProfileAnalyzer, ProfileController, ProfilePlot)

X_LENGTH, Y_LENGTH = (5, 4)


class TestProfileAuxPlot(GuiTestCase):

    def setUp(self):
        super().setUp()
        self._controller = ProfileAggregator()
        self._controller.set_axes(x_data=np.arange(X_LENGTH),
                                  y_data=np.arange(Y_LENGTH))
        self._controller.on_trait_change(self._mock_slot, "stats")

    def tearDown(self):
        super().tearDown()
        self._controller.on_trait_change(self._mock_slot, "stats", remove=True)
        self._emitted_value = None

    def _mock_slot(self, value):
        self._emitted_value = value

    def test_basics(self):
        assert len(self._controller.controllers) == 2
        for aux_plot in self._controller.controllers.values():
            assert isinstance(aux_plot, ProfileController)
            assert isinstance(aux_plot.plot, ProfilePlot)
            assert isinstance(aux_plot.analyzer, ProfileAnalyzer)

        assert not self._controller.show_stats

    @pytest.mark.filterwarnings("ignore::scipy.optimize.OptimizeWarning")
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

        assert region.valid() is valid
        self._controller._enable_fitting(fitted)
        self._controller.process(region)

    def _assert_analyze(self, valid=True, fitted=True):
        for controller in self._controller.controllers.values():
            plot_item, analyzer = controller.plot, controller.analyzer
            data_item, fit_item = plot_item._data_item, plot_item._fit_item

            # Check profile values
            x_data, y_data = analyzer._x_profile, analyzer._y_profile
            if valid:
                data_assertion = self.assertArrayIsNotEmpty
                # Add one more vestigial data at the end because of step plot
                x_data = np.hstack((x_data, [x_data[-1] + 1]))
            else:
                data_assertion = self.assertArrayIsEmpty

            data_assertion(x_data)
            data_assertion(x_data)
            np.testing.assert_array_equal(data_item.xData, x_data)
            np.testing.assert_array_equal(data_item.yData, y_data)

            # Check fit values
            fit_assertion = (self.assertArrayIsNotEmpty if fitted
                             else self.assertArrayIsEmpty)
            x_fit, y_fit = analyzer._x_fit, analyzer._y_fit

            fit_assertion(x_fit)
            fit_assertion(y_fit)
            np.testing.assert_array_equal(fit_item.xData, x_fit)
            np.testing.assert_array_equal(fit_item.yData, y_fit)

    @pytest.mark.filterwarnings("ignore::scipy.optimize.OptimizeWarning")
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
        if self._controller.show_stats:
            self._controller._enable_fitting(False)
            assert not self._controller.show_stats
            assert self._emitted_value is None

        # Show stats
        self._controller._enable_fitting(True)
        assert self._controller.show_stats
        assertion(self._emitted_value.html, '')

        # Hide stats
        self._controller._enable_fitting(False)
        assert not self._controller.show_stats
        assert self._emitted_value is None

    @staticmethod
    def assertArrayIsEmpty(array):
        np.testing.assert_array_equal(array, [])

    @staticmethod
    def assertArrayIsNotEmpty(array):
        np.testing.assert_raises(AssertionError,
                                 np.testing.assert_array_equal, array, [])
