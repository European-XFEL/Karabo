import numpy as np

MAX_PRECISION = -24


def float_to_string(f, precision=2):
    """Create pretty floats for legend items"""
    fmt = "{{:.{}f}}".format(precision)
    return fmt.format(f).rstrip("0").rstrip(".")


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
