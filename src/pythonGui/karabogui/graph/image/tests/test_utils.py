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
import pytest

from karabogui.graph.image.api import bytescale, rescale

SIZE = 10
LOW, HIGH = (-10.0, 10.0)
PRECISION_TOLERANCE = 4


def test_rescale():
    """Test the rescale function of the image utils"""

    def _assert_dtype(dtype):
        """Check according to different `dtype`"""
        min_level, max_level = (0, 127)
        image = np.linspace(min_level, max_level, endpoint=True,
                            num=SIZE, dtype=dtype)

        # Check rescale with the low and high as the data extrema.
        # Note: It should not rescale!
        rescaled = rescale(image, min_level, max_level,
                           low=min_level, high=max_level)
        # Use `array_almost_equal` here to avoid precision inequality
        np.testing.assert_almost_equal(image, rescaled,
                                       decimal=PRECISION_TOLERANCE)

        # Check different low and high
        rescaled = rescale(image, min_level, max_level, low=LOW, high=HIGH)
        assert rescaled[0] == LOW
        assert rescaled[-1] == HIGH

    _assert_dtype(np.uint8)
    _assert_dtype(np.int8)
    _assert_dtype(np.uint32)
    _assert_dtype(np.int32)
    _assert_dtype(np.float32)


def test_bytescale():
    """Test the `bytescale` function of the image utils"""

    min_level, max_level = (0, 127)
    image = np.linspace(min_level, max_level, endpoint=True,
                        num=SIZE, dtype=np.uint8)

    bytescaled = bytescale(image, cmin=min_level, cmax=max_level,
                           low=min_level, high=max_level)
    np.testing.assert_almost_equal(image, bytescaled,
                                   decimal=PRECISION_TOLERANCE)

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
