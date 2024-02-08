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
from scipy.optimize import curve_fit
from traits.api import (
    Any, Array, ArrayOrNone, Bool, Callable, Dict, Property, cached_property)

from karabogui.graph.common.api import ImageRegion
from karabogui.graph.common.fitting import gaussian_fit as gauss1d

from ..base.analyzer import BaseAnalyzer

FWHM_COEFF = 2 * np.sqrt(2 * np.log(2))


class ProfileAnalyzer(BaseAnalyzer):
    # The stats depends on the fit parameters
    stats = Property(Dict, depends_on='_fit_params')

    # Smoothen the resulting signal
    smooth = Bool(False)

    # Transformed image axis data
    axis_data = ArrayOrNone

    # The profiling function
    _profile_function = Callable(np.average)

    # --- Arrays ---
    # Resulting arrays from `analyze`
    _x_profile = Array
    _y_profile = Array

    # Resulting arrays from `fit`, which depends on the profile arrays
    _x_fit = Array
    _y_fit = Array

    # Fit parameters from the gaussian fit. Either tuple of values or None.
    _fit_params = Any

    # --- Convenience checkers ---
    has_profile = Property(Bool)
    has_fit = Property(Bool)

    # -----------------------------------------------------------------------
    # Public methods

    def analyze(self, region, axis=0):
        """Calculate the profile of the region on each axes."""
        x_profile, y_profile = np.array([]), np.array([])

        # Check if there's existing data. Bail out if None.
        if self.axis_data is not None:
            axis_slice = region.slices[axis]

            # Obtain the y-axis profile, which is dependent on the
            # image region type
            if region.region_type is ImageRegion.Area:
                y_profile = self._profile_function(region.region, axis=axis)
                x_profile = self.axis_data[axis_slice]
            elif region.region_type is ImageRegion.Line:
                x_profile = self.axis_data
                y_profile = region.region[axis]

                if self.smooth and len(y_profile) > 200:
                    y_profile = smooth_signal(y_profile)

        # Calculate the profile with the profiling function (e.g., sum, mean)
        self._x_profile, self._y_profile = x_profile, y_profile
        return x_profile, y_profile

    def fit(self):
        """Use Gaussian function to fit the profile"""
        # Calculate the offset of the x-axis with the half of the difference
        # of data points (it is assumed that the data is uniformly spaced)
        x_fit, y_fit = np.array([]), np.array([])
        fit_params = None

        if self.has_profile:
            offset = abs(self._x_profile[1] - self._x_profile[0]) / 2
            fit_params = gaussian_fit(self._x_profile, self._y_profile,
                                      offset=offset)

            if fit_params is not None:
                x_fit = self.axis_data + offset
                y_fit = gauss1d(self.axis_data, *fit_params)

        self._x_fit, self._y_fit = x_fit, y_fit
        self._fit_params = fit_params
        return x_fit, y_fit

    def clear_data(self):
        """Clear all the calculated arrays and the fit parameters"""
        self._x_profile = np.array([])
        self._y_profile = np.array([])
        self._x_fit = np.array([])
        self._y_fit = np.array([])
        self._fit_params = None

    # -----------------------------------------------------------------------
    # Trait properties

    @cached_property
    def _get_stats(self):
        stats = {}

        if self.has_profile:
            # Select data to calculate statistics on
            x_data, y_data = self._x_profile, self._y_profile
            if self.has_fit:
                x_data, y_data = self._x_fit, self._y_fit

            index_max = np.argmax(y_data)

            stats["max_pos"] = x_data[index_max]
            stats["amplitude"] = y_data[index_max]

            if self.has_fit:
                stats["fwhm"] = FWHM_COEFF * self._fit_params[2]

        return stats

    def _get_has_fit(self):
        return self._fit_params is not None

    def _get_has_profile(self):
        return bool(self._x_profile.size and self._y_profile.size)


def smooth_signal(y_profile):
    """We smooth the received data using the Moving Average approach so
    we can get rid of the up/down peaks on noisy images that make the
    plot very time-consuming. Ideally this should be done only on noisy
    images, so once we figure out a way to assert it we can avoid doing
    this everytime"""
    window = int(0.005 * len(y_profile))  # Window size of 0.5%
    window = max(3, window)
    y_profile = np.convolve(y_profile,
                            np.ones((window,)) / window,
                            mode='same')

    return y_profile


def gaussian_fit(x_data, y_data, offset=0):
    """
    Centre-of-mass and width. Lifted from image_processing.imageCentreofMass()
    """
    try:
        x0 = np.average(x_data, weights=y_data)
        sx = np.sqrt(np.average((x_data - x0) ** 2, weights=y_data))
    except ZeroDivisionError:
        # Weighting might fail with zero's
        return None

    # Gaussian fit
    p_0 = (y_data.max(), x0 + offset, sx, y_data[0])
    try:
        p_f, _ = curve_fit(gauss1d, x_data, y_data, p_0)
        return p_f
    except (RuntimeError, TypeError):
        return None
