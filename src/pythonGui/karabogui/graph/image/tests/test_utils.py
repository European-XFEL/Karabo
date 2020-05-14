import numpy as np
from numpy.testing import assert_almost_equal
from ..utils import rescale

SIZE = 10
LOW, HIGH = (-10.0, 10.0)
PRECISION_TOLERANCE = 4


def test_rescale():
    _assert_dtype(np.uint8)
    _assert_dtype(np.int8)
    _assert_dtype(np.uint32)
    _assert_dtype(np.int32)
    _assert_dtype(np.float32)


def _assert_dtype(dtype):
    """Check the rescale on varying dtype. We use `array_almost_equal`
     here to avoid precision inequality"""
    min_level, max_level = (0, 127)
    image = np.linspace(min_level, max_level, endpoint=True,
                        num=SIZE, dtype=dtype)

    # Check rescale with the low and high as the data extrema.
    # It should not rescale
    rescaled = rescale(image, min_level, max_level,
                       low=min_level, high=max_level)
    assert_almost_equal(image, rescaled, decimal=PRECISION_TOLERANCE)

    # Check different low and high
    rescaled = rescale(image, min_level, max_level, low=LOW, high=HIGH)
    assert rescaled[0] == LOW
    assert rescaled[-1] == HIGH
