import numpy as np

from unittest import TestCase, main

from ..utils import dtype_from_number


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


if __name__ == "__main__":
    main()
