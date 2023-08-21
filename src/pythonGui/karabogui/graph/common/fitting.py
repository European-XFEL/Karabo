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
from scipy.stats import norm


def gaussian_fit(x, height, x0, sigma, offset):
    return height * np.exp(-0.5 * ((x - x0) / sigma) ** 2) + offset


def normal_cdf(x, mu, sigma):
    """
    :param mu: Mean of the normal distribution.
    :type mu: float
    :param sigma: Standard deviation of the normal distribution.
    :type sigma: float
    """
    return norm.cdf(x, loc=mu, scale=sigma)


def linear_function_fit(x, m, b):
    """
    :param m: Slope of the linear function
    :type m: Float
    :param b: Intercept of the linear function
    :type b: Float
    """
    return m * x + b


def sqsech(x, height, x0, sx, offset):
    """Returns a squared hyperbolic secant  with the given parameters.
    The hyperbolic secant curve is in the form
    height / ((np.cosh((x0 - x) / sx)) ** 2)
    """

    return height / ((np.cosh((x0 - x) / sx)) ** 2) + offset


def guess_initial_parameters(x, y):
    """
    Generate initial parameter for 'curve_fit' function for gaussian and
    sech squared fitting
    """
    height_guess = max(y)
    x0_guess = x[np.argmax(y)]
    sigma_guess = np.std(x)
    offset_guess = np.min(y)
    return [height_guess, x0_guess, sigma_guess, offset_guess]
