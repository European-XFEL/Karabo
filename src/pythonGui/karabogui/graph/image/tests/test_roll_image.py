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
        self.assertEqual(len(image.data), 1)
        image.add(value)
        np.testing.assert_array_almost_equal(image.data[0], value)
        self.assertEqual(len(image.data), 2)

    def test_overflow(self):
        image = RollImage()
        size = 20
        value = np.array([1] * size)
        arange_value = np.arange(size)
        for i in range(image.stack + 1):
            image.add(arange_value)
        np.testing.assert_array_almost_equal(image.data[0], arange_value)
        self.assertEqual(len(image.data), image.stack)
        image.add(value)
        np.testing.assert_array_almost_equal(image.data[0], value)
        self.assertEqual(len(image.data), image.stack)
        np.testing.assert_array_almost_equal(image.data[1], arange_value)

    def test_invalid_values(self):
        image = RollImage()
        size = 20
        value = np.ones(size, dtype=np.float64)
        value[0] = np.inf
        image.add(value)

        # Check if image is finite
        self.assertTrue(np.all(np.isfinite(image.data)))
        # Check if inf is turned into zero
        value[0] = 0
        np.testing.assert_equal(image.data[0], value)

        # Add another vector, this time with multiple nans
        value = np.ones(size, dtype=np.float64)
        value[[3, 5]] = np.nan
        image.add(value)

        # Check if image is finite
        self.assertTrue(np.all(np.isfinite(image.data)))
        # Check if nans are turned into zero
        value[[3, 5]] = 0
        np.testing.assert_equal(image.data[0], value)

    def test_empty_values(self):
        image = RollImage()
        size = 20

        # Check empty value at first set
        empty = np.array([])
        image.add(empty)
        self.assertIsNone(image.data)

        # Check empty value at nth set
        value = np.ones(size, dtype=np.float64)
        image.add(value)
        image.add(value)
        image.add(value)
        image.add(empty)
        self.assertIsNone(image.data)
