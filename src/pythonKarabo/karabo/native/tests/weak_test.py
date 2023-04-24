# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import TestCase, main

from karabo.native import Weak


class Tests(TestCase):
    def test_attribute(self):

        class A:
            w = Weak()

        class B:
            pass

        sentinel = B()

        self.assertIsInstance(A.w, Weak)

        a = A()
        self.assertTrue(hasattr(a, "w"))
        self.assertIsNone(a.w)

        a.w = sentinel
        self.assertIs(a.w, sentinel)
        del sentinel
        self.assertIsNone(a.w)

        del a.w
        self.assertTrue(hasattr(a, "w"))
        self.assertIsNone(a.w)

        a.w = None
        self.assertIsNone(a.w)

        sentinel = B()
        a.w = sentinel
        self.assertIs(a.w, sentinel)
        del sentinel
        self.assertIs(a.w, None)


if __name__ == "__main__":
    main()
