import numpy as np
from numpy.testing import assert_almost_equal
import pytest

from ..utils import bytescale, rescale

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


def test_bytescale():
    min_level, max_level = (0, 127)
    image = np.linspace(min_level, max_level, endpoint=True,
                        num=SIZE, dtype=np.uint8)

    bytescaled = bytescale(image, cmin=min_level, cmax=max_level,
                           low=min_level, high=max_level)
    assert_almost_equal(image, bytescaled, decimal=PRECISION_TOLERANCE)

    bytescaled = bytescale(image, cmin=min_level, cmax=max_level,
                           low=5, high=100)
    assert bytescaled[0] == 5
    assert bytescaled[-1] == 100

    bytescaled = bytescale(image, cmin=None, cmax=None,
                           low=5, high=100)
    assert bytescaled[0] == 5
    assert bytescaled[-1] == 100

    image = np.linspace(min_level, max_level, endpoint=True,
                        num=SIZE, dtype=np.uint32)
    bytescaled = bytescale(image, cmin=min_level, cmax=max_level,
                           low=5, high=100)
    assert bytescaled[0] == 5
    assert bytescaled[-1] == 100

    image = np.linspace(min_level, max_level, endpoint=True,
                        num=SIZE, dtype=np.float32)
    bytescaled = bytescale(image, cmin=min_level, cmax=max_level,
                           low=5, high=100)
    assert bytescaled[0] == 5
    assert bytescaled[-1] == 100

    with pytest.raises(ValueError):
        # No negative low allowed
        bytescale(image, cmin=min_level, cmax=max_level,
                  low=-5, high=100)

    with pytest.raises(ValueError):
        # High must be higher than low
        bytescale(image, cmin=min_level, cmax=max_level,
                  low=127, high=100)
