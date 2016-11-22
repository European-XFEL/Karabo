from unittest import TestCase, main

from pint import DimensionalityError

from karabo.middlelayer import cross, dot, linspace, unit


class Tests(TestCase):
    def test_linspace(self):
        self.assertTrue(all(linspace(3, 5, 3) == [3, 4, 5]))
        self.assertTrue(all(linspace(0, 2, 3) == [0, 1, 2]))
        self.assertTrue(all(linspace(-2, 0, 3) == [-2, -1, 0]))

        self.assertTrue(all(linspace(3 * unit.m, 5 * unit.m, 3) / unit.m ==
                            [3, 4, 5]))
        self.assertTrue(all(linspace(0 * unit.m, 2 * unit.m, 3) / unit.m ==
                            [0, 1, 2]))
        self.assertTrue(all(linspace(-2 * unit.m, 0 * unit.m, 3) / unit.m ==
                            [-2, -1, 0]))
        self.assertTrue(all(linspace(0, 2 * unit.m, 3) / unit.m ==
                            [0, 1, 2]))
        self.assertTrue(all(linspace(-2 * unit.m, 0, 3) / unit.m ==
                            [-2, -1, 0]))

        with self.assertRaises(DimensionalityError):
            linspace(2 * unit.kg, 3 * unit.m, 3)

    def test_cross(self):
        self.assertTrue(all(cross([1, 2, 3], [4, 5, 6]) == [-3, 6, -3]))
        self.assertTrue(all(cross([1, 2, 3] * unit.m, [4, 5, 6]) ==
                            [-3, 6, -3] * unit.m))
        self.assertTrue(all(cross([1, 2, 3], [4, 5, 6] * unit.m) ==
                            [-3, 6, -3] * unit.m))
        self.assertTrue(all(cross([1, 2, 3] * unit.s, [4, 5, 6] * unit.m) ==
                            [-3, 6, -3] * unit.m * unit.s))

    def test_dot(self):
        self.assertEqual(dot([1, 2, 3], [4, 5, 6]), 32)
        self.assertEqual(dot([1, 2, 3] * unit.m, [4, 5, 6]), 32 * unit.m)
        self.assertEqual(dot([1, 2, 3], [4, 5, 6] * unit.m), 32 * unit.m)
        self.assertEqual(dot([1, 2, 3] * unit.s, [4, 5, 6] * unit.m),
                         32 * unit.m * unit.s)

if __name__ == "__main__":
    main()
