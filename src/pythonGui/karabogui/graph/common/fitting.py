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
