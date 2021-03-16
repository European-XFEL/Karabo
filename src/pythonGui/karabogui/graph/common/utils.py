import numpy as np

MAX_PRECISION = -24


def float_to_string(f):
    return "{:.2f}".format(f).rstrip('0').rstrip('.')


def safe_log10(value):
    """Return the max precision if we receive zero when evaluating a value
       with log10. This is to protect the GUI from -inf."""

    if np.isscalar(value):
        return np.log10(value) if value > 0 else MAX_PRECISION

    if not isinstance(value, np.ndarray):
        value = np.array(value)

    # Calculate log10 for valid values, replace with nans otherwise
    index = value > 0
    result = np.log10(value, where=index)
    result[~index] = np.nan
    return result


CLIP_SLOW = list(map(int, np.__version__.split("."))) >= [1, 17, 0]


def clip_array(value, vmin, vmax):
    """This is a temporal workaround for numpy `clip` performance regression

    See https://github.com/numpy/numpy/issues/14281 for discussion ...
    """
    if CLIP_SLOW:
        return np.core.umath.maximum(np.core.umath.minimum(value, vmax), vmin)

    return np.clip(value, vmin, vmax)
