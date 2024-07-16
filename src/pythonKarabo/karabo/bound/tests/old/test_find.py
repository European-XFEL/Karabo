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
from karabo.bound import Hash


def test_find_1():
    h = Hash("a.b.c1.d", 1)

    node = h.find("a.b.c1.d")
    assert node is not None
    assert node.getKey() == "d"
    assert node.getValue() == 1

    node = h.find("a.b.c1.f")
    assert node is None


def test_find_2():
    h = Hash("a.b.c", "1")

    node = h.find("a.b.c")
    assert node is not None
    node.setValue(2)
    assert h.get("a.b.c") == 2

    node = h.find("a.b.c", '/')
    assert node is None
