import numpy as np

MAX_PRECISION = -24


def float_to_string(f):
    return "{:.2f}".format(f).rstrip('0').rstrip('.')


def safe_log10(value):
    """Return the max precision if we receive zero when evaluating a value
       with log10. This is to protect the GUI from -inf."""

    if value == 0:
        return MAX_PRECISION

    return np.log10(value)
