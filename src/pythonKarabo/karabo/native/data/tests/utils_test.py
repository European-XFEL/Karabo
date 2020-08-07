import numpy as np

from unittest import TestCase, main

from karabo.native.data import dtype_from_number, get_image_data, Hash


class Tests(TestCase):
    def test_numpy_int32_dtype(self):
        dtype = dtype_from_number(12)
        self.assertEqual(dtype, np.int32)

    def test_numpy_object_dtype(self):
        dtype = dtype_from_number(1)
        self.assertEqual(dtype, np.object_)

    def test_numpy_int64_dtype(self):
        dtype = dtype_from_number(16)
        self.assertEqual(dtype, np.int64)

    def test_numpy_unknwon(self):
        dtype = dtype_from_number(39)
        self.assertEqual(dtype, np.object_)

    def test_get_image_data(self):
        h = Hash()
        h['data.image.pixels'] = Hash()
        h['data.image.pixels.data'] = np.array([[2, 4, 6], [6, 8, 10]],
                                               np.int64)
        h['data.image.pixels.type'] = 16
        h['data.image.pixels.shape'] = np.array([2, 3], np.uint64)
        h['data.image.pixels.isBigEndian'] = False
        image = get_image_data(h)
        np.testing.assert_array_equal(
            image, np.array([[2, 4, 6], [6, 8, 10]], np.int64))


if __name__ == "__main__":
    main()
