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
        self.assertFalse(hasattr(a, "w"))
        a.w = sentinel
        self.assertIs(a.w, sentinel)
        del sentinel
        self.assertIs(a.w, None)
        del a.w
        self.assertFalse(hasattr(a, "w"))

        a.w = None
        self.assertIs(a.w, None)

        sentinel = B()
        a.w = sentinel
        self.assertIs(a.w, sentinel)
        del sentinel
        self.assertIs(a.w, None)


if __name__ == "__main__":
    main()
