# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
