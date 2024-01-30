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
import unittest

import numpy as np

from karabogui.graph.image.roll_image import RollImage


class TestRollImage(unittest.TestCase):
    def test_create(self):
        image = RollImage()
        size = 20
        value = np.array([1] * size)
        image.add(np.arange(size))
        np.testing.assert_array_almost_equal(image.data[0], np.arange(size))
        assert len(image.data) == 1
        image.add(value)
        np.testing.assert_array_almost_equal(image.data[0], value)
        assert len(image.data) == 2

    def test_overflow(self):
        image = RollImage()
        size = 20
        value = np.array([1] * size)
        arange_value = np.arange(size)
        for i in range(image.stack + 1):
            image.add(arange_value)
        np.testing.assert_array_almost_equal(image.data[0], arange_value)
        assert len(image.data) == image.stack
        image.add(value)
        np.testing.assert_array_almost_equal(image.data[0], value)
        assert len(image.data) == image.stack
        np.testing.assert_array_almost_equal(image.data[1], arange_value)

    def test_invalid_values(self):
        image = RollImage()
        size = 20
        value = np.ones(size, dtype=np.float64)
        value[0] = np.inf
        image.add(value)

        # Check if image is finite
        assert np.all(np.isfinite(image.data))
        # Check if inf is turned into zero
        value[0] = 0
        np.testing.assert_equal(image.data[0], value)

        # Add another vector, this time with multiple nans
        value = np.ones(size, dtype=np.float64)
        value[[3, 5]] = np.nan
        image.add(value)

        # Check if image is finite
        assert np.all(np.isfinite(image.data))
        # Check if nans are turned into zero
        value[[3, 5]] = 0
        np.testing.assert_equal(image.data[0], value)

    def test_empty_values(self):
        image = RollImage()
        size = 20

        # Check empty value at first set
        empty = np.array([])
        image.add(empty)
        assert image.data is None

        # Check empty value at nth set
        value = np.ones(size, dtype=np.float64)
        image.add(value)
        image.add(value)
        image.add(value)
        image.add(empty)
        assert image.data is None

    def test_stack_change_reset(self):
        image = RollImage()
        size = 20
        arange_value = np.arange(size)
        for i in range(image.stack + 1):
            image.add(arange_value)
        assert image.data is not None
        # Stack is 100 by default
        image.stack = 100
        assert image.data is not None
        image.stack = 20
        assert image.data is None
        arange_value = np.arange(size)
        for i in range(image.stack + 1):
            image.add(arange_value)
        assert image.data is not None
        image.reset()
        assert image.data is None
