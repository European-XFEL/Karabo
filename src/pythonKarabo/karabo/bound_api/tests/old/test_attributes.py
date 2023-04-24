# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from karabo.bound import Hash, Types


def test_attributes_1():
    h = Hash("a.b.a.b", 42)
    h.setAttribute("a.b.a.b", "attr1", "someValue")
    assert h.getAttribute("a.b.a.b", "attr1") == "someValue"


def test_attributes_2():
    h = Hash("a.b.a.b", 42)
    h.setAttribute("a.b.a.b", "attr1", "someValue")
    assert h.getAttribute("a.b.a.b", "attr1") == "someValue"

    h.setAttribute("a.b.a.b", "attr2", 42)
    assert h.getAttribute("a.b.a.b", "attr1") == "someValue"
    assert h.getAttribute("a.b.a.b", "attr2") == 42

    h.setAttribute("a.b.a.b", "attr2", 43)
    assert h.getAttribute("a.b.a.b", "attr1") == "someValue"
    assert h.getAttribute("a.b.a.b", "attr2") == 43

    h.setAttribute("a.b.a.b", "attr1", True)
    assert h.getAttribute("a.b.a.b", "attr1")
    assert h.getAttribute("a.b.a.b", "attr2") == 43

    attrs = h.getAttributes("a.b.a.b")
    assert attrs.size() == 2
    assert attrs.get("attr1")
    assert attrs.get("attr2") == 43

    node = attrs.getNode("attr1")
    assert node.getType() == Types.BOOL

    node = attrs.getNode("attr2")
    assert node.getType() == Types.INT32


def test_attributes_3():
    h = Hash("a.b.a.b", 1)
    h.setAttribute("a.b.a.b", "attr1", [1, 2, 3])
    assert h.getAttribute("a.b.a.b", "attr1")[1], 2


def test_attributes_4():
    h = Hash("a.b.a.b", 42)
    h.setAttribute("a.b.a.b", "attr1", "someValue")
    assert h.getAttribute("a.b.a.b", "attr1") == "someValue"


def test_attributes_5():
    h = Hash("a.b.a.b", 42)
    h.setAttribute("a.b.a.b", "attr1", "someValue")
    assert h.getAttribute("a.b.a.b", "attr1") == "someValue"
