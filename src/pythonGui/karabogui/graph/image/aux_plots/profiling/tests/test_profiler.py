from unittest import TestCase

import numpy as np

from karabogui.graph.common.api import ImageRegion

from ..profiler import IntensityProfiler

X_LENGTH, Y_LENGTH = (5, 4)


class TestIntensityProfiler(TestCase):

    def setUp(self):
        self._profiler = IntensityProfiler()

    def test_basics(self):
        self.assertIsNone(self._profiler._data)
        self.assertIsNone(self._profiler._fit)
        self.assertIsNone(self._profiler._fit_params)
        self.assertIsNone(self._profiler._axis)
        self.assertFalse(self._profiler._smooth)

    def test_profile(self):
        # Check default, fails without axis data
        self._profiler.profile(self._get_region(valid=True))
        self.assertIsNone(self._profiler._data)

        # Now add axis data, then perform profiling
        self._profiler.set_axis(np.arange(X_LENGTH))
        self._profiler.profile(self._get_region(valid=True), axis=0)
        self.assertIsNotNone(self._profiler._data)

        # Check invalid region
        self._profiler.profile(self._get_region(valid=False))
        self.assertIsNone(self._profiler._data)

    def test_fit(self):
        # Check default, fails without axis data and profile data
        self._profiler.fit()
        self.assertIsNone(self._profiler._fit)
        self.assertIsNone(self._profiler._fit_params)

        # Now add axis data, should still fail since no profile data
        self._profiler.set_axis(np.arange(X_LENGTH))
        self._profiler.fit()
        self.assertIsNone(self._profiler._fit)
        self.assertIsNone(self._profiler._fit_params)

        # Now perform profiling with valid region
        self._profiler.profile(self._get_region(valid=True), axis=0)
        self._profiler.fit()
        self.assertIsNotNone(self._profiler._fit)
        self.assertIsNotNone(self._profiler._fit_params)

        # Now perform profiling with valid invalid region
        self._profiler.profile(self._get_region(valid=False), axis=0)
        self._profiler.fit()
        self.assertIsNone(self._profiler._fit)
        self.assertIsNone(self._profiler._fit_params)

    def test_get_stats(self):
        # Check default, fails without axis data and profile data
        stats = self._profiler.get_stats()
        self.assertIsNone(stats)

        # Now add axis data, should still fail since no profile data
        self._profiler.set_axis(np.arange(X_LENGTH))
        stats = self._profiler.get_stats()
        self.assertIsNone(stats)

        # Now perform profiling with valid region, but not fitted
        self._profiler.profile(self._get_region(valid=True), axis=0)
        stats = self._profiler.get_stats()
        self.assertIsNotNone(stats)
        self.assertIsNone(stats.get("fwhm"))

        # Now perform profiling with valid region and fitted
        self._profiler.profile(self._get_region(valid=True), axis=0)
        self._profiler.fit()
        stats = self._profiler.get_stats()
        self.assertIsNotNone(stats)
        self.assertIsNotNone(stats.get("fwhm"))

        # Now perform profiling with valid invalid region
        self._profiler.profile(self._get_region(valid=False), axis=0)
        self._profiler.fit()
        self.assertIsNone(self._profiler._fit)
        self.assertIsNone(self._profiler._fit_params)

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
