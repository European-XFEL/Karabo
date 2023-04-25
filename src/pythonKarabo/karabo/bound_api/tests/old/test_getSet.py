# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from karabo.bound import Hash, Types


def test_getSet_1():
    h = Hash()
    h.set("a.b.c1.d", 1)

    assert h.get("a").has("b")
    assert h.get("a.b").has("c1")
    assert h.get("a.b.c1").has("d")
    assert h.get("a.b.c1.d") == 1
    assert h.has("a.b.c1.d")
    assert "a.b.c1.d" in h
    assert h.get("a").has("b.c1")
    assert "b.c1" in h["a"]

    h.set("a.b.c2.d", 2.0)

    assert h.get("a.b").has("c2.d")
    assert h.get("a.b").has("c1.d")
    assert h.get("a.b.c1.d") == 1
    assert h.get("a.b.c2.d") == 2.0

    h.set("a.b[0]", Hash("a", 1))

    assert h.get("a").has("b")
    assert h["a"].has("b")
    assert "b" in h["a"]
    assert len(h.get("a")) == 1
    assert len(h["a"]) == 1
    assert len(h.get("a.b")) == 1
    assert len(h["a.b"]) == 1
    assert len(h.get("a.b")[0]) == 1
    assert len(h["a.b"][0]) == 1
    assert h["a.b"][0]["a"] == 1
    assert h["a.b[0].a"] == 1
    assert h.get("a.b")[0].get("a") == 1
    assert h.get("a.b[0].a") == 1

    h.set("a.b[2]", Hash("a", 1))

    assert h.get("a").has("b")
    assert len(h["a"]) == 1
    assert len(h["a.b"]) == 3
    assert h["a.b[0].a"] == 1
    assert h["a.b[2].a"] == 1
    assert h["a.b"][0]["a"] == 1
    assert h["a.b"][2]["a"] == 1
    assert h["a.b"][1].empty()


def test_getSet_2():
    h = Hash()
    h["a.b.c"] = 1
    h["a.b.c"] = 2

    assert h["a.b.c"] == 2
    assert h.has("a.b")
    assert not h.has("a.b.c.d")


def test_getSet_3():
    h = Hash("a[0]", Hash("a", 1), "a[1]", Hash("a", 1))

    assert h["a[0].a"] == 1
    assert h["a[1].a"] == 1
    assert h["a"][0]["a"] == 1
    assert h["a"][1]["a"] == 1


def test_getSet_4():
    h = Hash()
    h["x[0].y[0]"] = Hash("a", 4.2, "b", "red", "c", True)
    h["x[1].y[0]"] = Hash("a", 4.0, "b", "green", "c", False)

    assert h["x[0].y[0].c"]
    assert not h["x[1].y[0].c"]
    assert h["x[0].y[0].b"] == "red"
    assert h["x[1].y[0].b"] == "green"


def test_getSet_5():
    h1 = Hash("a[0].b[0]", Hash("a", 1))
    h2 = Hash("a[0].b[0]", Hash("a", 2))

    assert h1["a[0].b[0].a"] == 1
    h1["a[0]"] = h2
    assert h1["a[0].a[0].b[0].a"] == 2
    h1["a"] = h2
    assert h1["a.a[0].b[0].a"] == 2


def test_getSet_6():
    h = Hash()
    b = True
    h["a"] = b

    assert str(h.getType("a")) == "BOOL"
    assert h.getType("a") == Types.BOOL
