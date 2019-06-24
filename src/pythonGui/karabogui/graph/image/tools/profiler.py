import numpy as np
from scipy.optimize import curve_fit

from karabogui.graph.common.api import ImageRegion


class IntensityProfiler:

    def __init__(self, profiler=np.sum):
        self._profiler = profiler
        self._profile = None
        self._fit = None

    def profile(self, region, axis=0):
        """Calculate the profile of the region on each axes."""

        # Clear previous data
        self._profile = None
        self._fit = None

        # Obtain the y-axis profile, which is dependent on the
        # image region type
        if region.region_type is ImageRegion.Area:
            y_profile = self._profiler(region.region, axis=axis)
        elif region.region_type is ImageRegion.Line:
            y_profile = region.region[axis]
        else:
            return

        # Calculate the profile with the profiling function (e.g., sum, mean)
        x_profile = region.axes[axis]
        self._profile = (x_profile, y_profile)

        return self._profile

    def fit(self):
        """Use Gaussian function to fit the profile"""

        # Calculate the offset of the x-axis with the half of the difference
        # of data points (it is assumed that the data is uniformly spaced)
        offset = abs(np.diff(self._profile[0][:2])[0]) / 2
        x_data, y_data = self._profile
        self._fit = gaussian_fit(x_data, y_data, offset)
        return self._fit

    def analyze(self):
        """Get peak parameters from the fit (if fitted) or from the profile"""
        fitted = self._has_fit()
        data = self._fit if fitted else self._profile
        return peak_parameters(*data, fitted=fitted)

    def _has_fit(self):
        """Check if there is an existing fit:
           - if self.fit() is invoked
           - if gaussian fit returns a valid result"""
        return self._fit is not None and self._fit[1].size


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
        return x_data + offset, gauss1d(x_data, *p_f)
    except (RuntimeError, TypeError):
        return np.array([]), np.array([])


def gauss1d(x, height, x0, sigma, offset):
    return height * np.exp(-0.5 * ((x - x0) / sigma) ** 2) + offset


def peak_parameters(x_data, y_data, fitted=True):
    """Modified image_processing.peakParametersEval()"""
    fwhm = None
    data = np.copy(y_data)

    # subtract pedestal
    pedestal = data.min()
    data -= pedestal

    index_max = data.argmax()

    maxPos = x_data[index_max]
    ampl = float(data.max())

    if fitted:
        half_maximum = ampl / 2

        if 0 < index_max < len(data):
            halfmaxPosHi = (np.abs(data[index_max:] - half_maximum).argmin() +
                            index_max)
            halfmaxPosLo = np.abs(data[:index_max] - half_maximum).argmin()
            if 0 <= halfmaxPosLo < index_max < halfmaxPosHi < len(data):
                index_fwhm = int(halfmaxPosHi - halfmaxPosLo)
                fwhm = x_data[index_fwhm]

    return [ampl, maxPos, fwhm]
