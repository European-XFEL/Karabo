# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
