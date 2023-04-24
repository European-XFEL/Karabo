# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import TestCase, main

from pint import DimensionalityError

from karabo.middlelayer import Int32, QuantityValue, numeric, unit


class Tests(TestCase):
    def setUp(self):
        self.ts1 = 1 * unit.m
        self.ts1.descriptor = Int32()
        self.ts1.timestamp = 100
        self.ts2 = self.ts1 / unit.m
        self.ts2.timestamp = 200
        self.nounit = QuantityValue(3, timestamp=300)

    def test_linspace(self):
        self.assertTrue(all(numeric.linspace(3, 5, 3) == [3, 4, 5]))
        self.assertTrue(all(numeric.linspace(0, 2, 3) == [0, 1, 2]))
        self.assertTrue(all(numeric.linspace(-2, 0, 3) == [-2, -1, 0]))

        self.assertTrue(all(numeric.linspace(3 * unit.m, 5 * unit.m, 3) /
                            unit.m == [3, 4, 5]))
        self.assertTrue(all(numeric.linspace(0 * unit.m, 2 * unit.m, 3) /
                            unit.m == [0, 1, 2]))
        self.assertTrue(all(numeric.linspace(-2 * unit.m, 0 * unit.m, 3) /
                            unit.m == [-2, -1, 0]))
        self.assertTrue(all(numeric.linspace(3 * unit.m / unit.m, 5, 3) ==
                            [3, 4, 5]))
        self.assertTrue(all(numeric.linspace(3, 5 * unit.m / unit.m, 3) ==
                            [3, 4, 5]))

        self.assertEqual(numeric.linspace(self.ts1, 3 * self.ts1,
                                          self.nounit).timestamp, 300)

        with self.assertRaises(DimensionalityError):
            numeric.linspace(2 * unit.kg, 3 * unit.m, 3)

    def test_cross(self):
        self.assertTrue(all(numeric.cross([1, 2, 3], [4, 5, 6]) ==
                            [-3, 6, -3]))
        self.assertTrue(all(numeric.cross([1, 2, 3] * unit.m, [4, 5, 6]) ==
                            [-3, 6, -3] * unit.m))
        self.assertTrue(all(numeric.cross([1, 2, 3], [4, 5, 6] * unit.m) ==
                            [-3, 6, -3] * unit.m))
        self.assertTrue(
            all(numeric.cross([1, 2, 3] * unit.s, [4, 5, 6] * unit.m) ==
                [-3, 6, -3] * unit.m * unit.s))
        self.assertEqual(numeric.cross([1, 2, 3] * self.ts1,
                                       [4, 5, 6] * self.ts2).timestamp, 200)

    def test_dot(self):
        self.assertEqual(numeric.dot([1, 2, 3], [4, 5, 6]), 32)
        self.assertEqual(numeric.dot([1, 2, 3] * unit.m, [4, 5, 6]),
                         32 * unit.m)
        self.assertEqual(numeric.dot([1, 2, 3], [4, 5, 6] * unit.m),
                         32 * unit.m)
        self.assertEqual(numeric.dot([1, 2, 3] * unit.s, [4, 5, 6] * unit.m),
                         32 * unit.m * unit.s)
        self.assertEqual(numeric.dot([1, 2, 3] * self.ts1,
                                     [4, 5, 6] * self.ts2).timestamp, 200)


if __name__ == "__main__":
    main()
