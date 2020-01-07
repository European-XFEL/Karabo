import numpy as np
from scipy.optimize import curve_fit

from karabogui.graph.common.api import ImageRegion

from ..base import BaseAuxPlotAnalyzer


FWHM_COEFF = 2 * np.sqrt(2 * np.log(2))


class IntensityProfiler(BaseAuxPlotAnalyzer):

    def __init__(self, smooth=False, profiler=np.average):
        self._profile_function = profiler
        self._data = None
        self._fit = None  # fit values wrt to data
        self._fit_params = None  # fit parameters from gaussian fit
        self._axis = None  # transformed axis data

        self._smooth = smooth

    def profile(self, region, axis=0):
        """Calculate the profile of the region on each axes."""

        # Check if there's existing data. Bail out if None.
        if self._axis is None:
            return

        # Clear previous data
        self.clear_data()

        axis_slice = region.slices[axis]

        # Obtain the y-axis profile, which is dependent on the
        # image region type
        if region.region_type is ImageRegion.Area:
            y_profile = self._profile_function(region.region, axis=axis)
            x_profile = self._axis[axis_slice]
        elif region.region_type is ImageRegion.Line:
            x_profile = self._axis
            y_profile = region.region[axis]

            if self._smooth and len(y_profile) > 200:
                y_profile = self._smooth_signal(y_profile)
        else:
            return

        # Calculate the profile with the profiling function (e.g., sum, mean)
        self._data = (x_profile, y_profile)

        return x_profile, y_profile

    def _smooth_signal(self, y_profile):
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

    def fit(self):
        """Use Gaussian function to fit the profile"""
        # Calculate the offset of the x-axis with the half of the difference
        # of data points (it is assumed that the data is uniformly spaced)
        if self._data is None:
            return np.array([]), np.array([])

        x_data, y_data = self._data
        offset = abs(np.diff(x_data[:2])[0]) / 2
        self._fit_params = gaussian_fit(x_data, y_data, offset)

        self._fit = np.array([]), np.array([])
        if self._fit_params is not None:
            x_fit = self._axis + offset
            y_fit = gauss1d(self._axis, *self._fit_params)
            self._fit = (x_fit, y_fit)

        return self._fit

    def get_stats(self):
        if self._data is None:
            return

        stats = {}

        fitted = self._fit_params is not None
        x_data, y_data = self._fit if fitted else self._data
        index_max = np.argmax(y_data)

        stats["max_pos"] = x_data[index_max]
        stats["amplitude"] = y_data[index_max]

        if fitted:
            stats["fwhm"] = FWHM_COEFF * self._fit_params[2]

        return stats

    def set_axis(self, axis):
        """Transformed axis data, used to obtain the profile from the image
           region slices. Set when the image changes transform and/or size."""
        self._axis = axis

    def clear_data(self):
        self._data = None
        self._fit = None
        self._fit_params = None


def gaussian_fit(x_data, y_data, offset):
    """
    Centre-of-mass and width. Lifted from image_processing.imageCentreofMass()
    """

    x0 = np.average(x_data, weights=y_data)
    sx = np.sqrt(np.average((x_data - x0) ** 2, weights=y_data))

    # Gaussian fit
    p_0 = (y_data.max(), x0 + offset, sx, y_data[0])
    try:
        p_f, _ = curve_fit(gauss1d, x_data, y_data, p_0)
        return p_f
    except (RuntimeError, TypeError):
        return None


def gauss1d(x, height, x0, sigma, offset):
    return height * np.exp(-0.5 * ((x - x0) / sigma) ** 2) + offset
