import numpy as np


def gaussian_fit(x, height, x0, sigma, offset):
    return height * np.exp(-0.5 * ((x - x0) / sigma) ** 2) + offset
