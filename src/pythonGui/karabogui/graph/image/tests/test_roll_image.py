import unittest

import numpy as np

from karabogui.graph.image.roll_image import RollImage


class TestRollImage(unittest.TestCase):
    def test_create(self):
        image = RollImage()
        size = 20
        value = np.array([1] * size)
        image.add(np.arange(size))
        print(image.data)
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

