# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import TestCase

import numpy as np

from karabogui.graph.common.api import ImageRegion

from ..analyzer import ProfileAnalyzer

X_LENGTH, Y_LENGTH = (5, 4)


class TestIntensityProfiler(TestCase):

    def setUp(self):
        self._analyzer = ProfileAnalyzer()

    def test_basics(self):
        self.assertFalse(self._analyzer.smooth)
        self.assertIsNone(self._analyzer.axis_data)
        self.assertArrayIsEmpty(self._analyzer._x_profile)
        self.assertArrayIsEmpty(self._analyzer._y_profile)
        self.assertArrayIsEmpty(self._analyzer._x_fit)
        self.assertArrayIsEmpty(self._analyzer._y_fit)
        self.assertArrayIsEmpty(self._analyzer._fit_params)

    def test_analyze(self):
        # Check default, fails without axis data
        self._analyzer.analyze(self._get_region(valid=True))
        self.assertArrayIsEmpty(self._analyzer._x_profile)
        self.assertArrayIsEmpty(self._analyzer._y_profile)

        # Now add axis data, then perform profiling
        self._analyzer.axis_data = np.arange(X_LENGTH)
        self._analyzer.analyze(self._get_region(valid=True), axis=0)
        self.assertArrayIsNotEmpty(self._analyzer._x_profile)
        self.assertArrayIsNotEmpty(self._analyzer._y_profile)

        # Check invalid region
        self._analyzer.analyze(self._get_region(valid=False))
        self.assertArrayIsEmpty(self._analyzer._x_profile)
        self.assertArrayIsEmpty(self._analyzer._y_profile)

    def test_fit(self):
        # Check default, fails without axis data and profile data
        self._analyzer.fit()
        self.assertArrayIsEmpty(self._analyzer._x_fit)
        self.assertArrayIsEmpty(self._analyzer._y_fit)
        self.assertIsNone(self._analyzer._fit_params)

        # Now add axis data, should still fail since no profile data
        self._analyzer.axis_data = np.arange(X_LENGTH)
        self._analyzer.fit()
        self.assertArrayIsEmpty(self._analyzer._x_fit)
        self.assertArrayIsEmpty(self._analyzer._y_fit)
        self.assertIsNone(self._analyzer._fit_params)

        # Now perform profiling with valid region
        self._analyzer.analyze(self._get_region(valid=True), axis=0)
        self._analyzer.fit()
        self.assertArrayIsNotEmpty(self._analyzer._x_fit)
        self.assertArrayIsNotEmpty(self._analyzer._y_fit)
        self.assertIsNotNone(self._analyzer._fit_params)

        # Now perform profiling with valid invalid region
        self._analyzer.analyze(self._get_region(valid=False), axis=0)
        self._analyzer.fit()
        self.assertArrayIsEmpty(self._analyzer._x_fit)
        self.assertArrayIsEmpty(self._analyzer._y_fit)
        self.assertIsNone(self._analyzer._fit_params)

    def test_get_stats(self):
        # Check default, fails without axis data and profile data
        self.assertEqual(self._analyzer.stats, {})

        # Now add axis data, should still fail since no profile data
        self._analyzer.axis_data = np.arange(X_LENGTH)
        self.assertDictEqual(self._analyzer.stats, {})

        # Now perform profiling with valid region and fitted
        self._analyzer.analyze(self._get_region(valid=True), axis=0)
        self._analyzer.fit()
        stats = self._analyzer.stats
        self.assertNotEqual(stats, {})
        self.assertIsNotNone(stats.get("fwhm"))

        # Now perform profiling with valid invalid region
        self._analyzer.analyze(self._get_region(valid=False), axis=0)
        self._analyzer.fit()
        stats = self._analyzer.stats
        self.assertEqual(stats, {})
        self.assertIsNone(stats.get("fwhm"))

    def _get_region(self, valid=True):
        region = ImageRegion()
        if valid:
            image = np.arange(X_LENGTH * Y_LENGTH).reshape(Y_LENGTH, X_LENGTH)
            region = ImageRegion(region=image,
                                 region_type=ImageRegion.Area,
                                 x_slice=slice(X_LENGTH),
                                 y_slice=slice(Y_LENGTH))

        self.assertTrue(region.valid() is valid)
        return region

    @staticmethod
    def assertArrayIsEmpty(array):
        np.testing.assert_array_equal(array, [])

    @staticmethod
    def assertArrayIsNotEmpty(array):
        np.testing.assert_raises(AssertionError,
                                 np.testing.assert_array_equal, array, [])
