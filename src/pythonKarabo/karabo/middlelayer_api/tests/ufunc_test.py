from unittest import TestCase, main

from pint import DimensionalityError

from karabo.middlelayer import unit
from karabo.middlelayer import numeric

import numpy as np


class Tests(TestCase):
    def setUp(self):
        self.N = 3
        self.ts1 = 1 * unit.m
        # self.ts1.timestamp = 100
        self.arr1 = np.array([self.ts1 * x for x in range(self.N)])

        # self.ts2 = self.ts1 / unit.m
        # self.ts2.timestamp = 200

    def assertArrayEqual(self, a1, a2):
        self.assertTupleEqual(tuple(a1), tuple(a2))
        # doesn't work:
        # ts1 = [x.timestamp for x in a1]
        # ts2 = [x.timestamp for x in a2]
        # self.assertListEqual(ts1, ts2)

    def test_assertArray(self):
        self.assertArrayEqual(self.arr1, self.arr1)
        self.assertArrayEqual(np.array([x / unit.m for x in self.arr1]),
                              np.array(range(self.N)))
        try:
            self.assertArrayEqual(self.arr1 + [1 * unit.m], self.arr1)
            raise ValueError
        except AssertionError:
            pass

    def test_add(self):
        m1 = self.ts1
        mm = m1 / 1000
        self.assertArrayEqual(np.add(self.arr1, self.arr1), 2 * self.arr1)
        self.assertArrayEqual(np.add(self.arr1, [0] * self.N), self.arr1)
        self.assertArrayEqual(np.add(self.arr1, [1 * unit.mm]*self.N),
                              np.array([x * m1 + mm for x in range(self.N)]))


if __name__ == "__main__":
    main()

